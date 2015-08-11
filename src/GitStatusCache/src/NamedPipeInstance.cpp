#include "stdafx.h"
#include "NamedPipeInstance.h"

void NamedPipeInstance::OnClientRequest()
{
	Log("NamedPipeInstance.OnClientRequest.Start", Severity::Verbose) << "Request servicing thread started.";

	while (true)
	{
		auto readResult = ReadRequest();
		if (readResult.first != IoResult::Success)
		{
			break;
		}

		Log("NamedPipeInstance.OnClientRequest.Request", Severity::Spam)
			<< LR"(Received request from client. { "request": ")" << readResult.second << LR"(" })";

		auto response = m_onClientRequestCallback(readResult.second);

		Log("NamedPipeInstance.OnClientRequest.Response", Severity::Spam)
			<< LR"(Sending response to client. { "response": ")" << response << LR"(" })";

		auto writeResult = WriteResponse(response);
		if (writeResult != IoResult::Success)
		{
			break;
		}
	}

	::FlushFileBuffers(m_pipe);
	::DisconnectNamedPipe(m_pipe);
	m_pipe.invoke();
	m_isClosed = true;

	Log("NamedPipeInstance.OnClientRequest.Stop", Severity::Verbose) << "Request servicing thread stopping.";
}

NamedPipeInstance::ReadResult NamedPipeInstance::ReadRequest()
{
	auto requestBuffer = std::vector<char>(BufferSize);
	DWORD bytesRead = 0;
	auto readResult = ::ReadFile(
		m_pipe,
		requestBuffer.data(),
		requestBuffer.capacity() * sizeof(char),
		&bytesRead,
		nullptr /*lpOverlapped*/);

	if (!readResult)
	{
		auto error = ::GetLastError();
		if (error == ERROR_BROKEN_PIPE)
		{
			Log("NamedPipeInstance.ReadRequest.Disconnect", Severity::Verbose)
				<< "Client disconnected. ReadFile returned ERROR_BROKEN_PIPE.";
			return ReadResult(IoResult::Aborted, std::string());
		}
		else if (error == ERROR_OPERATION_ABORTED)
		{
			Log("NamedPipeInstance.ReadRequest.Aborted", Severity::Verbose) << "ReadFile returned ERROR_OPERATION_ABORTED.";
			return ReadResult(IoResult::Aborted, std::string());
		}
		else
		{
			Log("NamedPipeInstance.ReadRequest.UnknownError", Severity::Error)
				<< R"(ReadFile failed with unexpected error. { "error": )" << error << R"( })";
			throw std::runtime_error("ReadFile failed unexpectedly.");
			return ReadResult(IoResult::Error, std::string());
		}
	}

	return ReadResult(IoResult::Success, std::string(requestBuffer.data(), bytesRead / sizeof(char)));
}

NamedPipeInstance::IoResult NamedPipeInstance::WriteResponse(const std::string& response)
{
	DWORD bytesWritten = 0;
	auto writeResult = ::WriteFile(
		m_pipe,
		response.data(),
		response.size() * sizeof(char),
		&bytesWritten,
		nullptr /*lpOverlapped*/);

	if (!writeResult)
	{
		auto error = ::GetLastError();
		if (error == ERROR_BROKEN_PIPE)
		{
			Log("NamedPipeInstance.WriteResponse.Disconnect", Severity::Verbose)
				<< "Client disconnected. WriteFile returned ERROR_BROKEN_PIPE.";
			return IoResult::Aborted;
		}
		else if (error == ERROR_OPERATION_ABORTED)
		{
			Log("NamedPipeInstance.WriteResponse.Aborted", Severity::Verbose) << "WriteFile returned ERROR_OPERATION_ABORTED.";
			return IoResult::Aborted;
		}
		else
		{
			Log("NamedPipeInstance.WriteResponse.UnknownError", Severity::Error)
				<< R"(WriteFile failed with unexpected error. { "error": )" << error << R"( })";
			throw std::runtime_error("WriteFile failed unexpectedly.");
			return IoResult::Error;
		}
	}

	return IoResult::Success;
}

NamedPipeInstance::NamedPipeInstance(const OnClientRequestCallback& onClientRequestCallback)
	: m_onClientRequestCallback(onClientRequestCallback)
	, m_pipe(MakeUniqueHandle(INVALID_HANDLE_VALUE))
{
	auto pipeMode = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT;
	auto timeout = 0;
	auto pipe = ::CreateNamedPipe(
		L"\\\\.\\pipe\\GitStatusCache",
		PIPE_ACCESS_DUPLEX,
		pipeMode,
		PIPE_UNLIMITED_INSTANCES,
		BufferSize,
		BufferSize,
		timeout,
		nullptr);

	if (pipe == INVALID_HANDLE_VALUE)
	{
		Log("NamedPipeInstance.Create", Severity::Error) << "Failed to create named pipe instance.";
		throw std::runtime_error("Failed to create named pipe instance.");
	}
	m_pipe = MakeUniqueHandle(pipe);
}

NamedPipeInstance::~NamedPipeInstance()
{
	if (m_thread.joinable())
	{
		if (!m_isClosed)
		{
			Log("NamedPipeInstance.ShutDown.StoppingBackgroundThread", Severity::Spam)
				<< R"(Shutting down request servicing thread. { "threadId": 0x)" << std::hex << m_thread.get_id() << " }";
			::CancelSynchronousIo(m_thread.native_handle());
		}
		m_thread.join();
	}
}

NamedPipeInstance::IoResult NamedPipeInstance::Connect()
{
	auto connected = ::ConnectNamedPipe(m_pipe, nullptr /*lpOverlapped*/);
	if (connected || ::GetLastError() == ERROR_PIPE_CONNECTED)
	{
		Log("NamedPipeInstance.Connect.Success", Severity::Spam) << "ConnectNamedPipe succeeded.";

		std::call_once(m_flag, [this]()
		{
			Log("NamedPipeInstance.StartingBackgroundThread", Severity::Spam)
				<< "Attempting to start background thread for servicing requests.";
			m_thread = std::thread(&NamedPipeInstance::OnClientRequest, this);
		});

		return IoResult::Success;
	}

	auto error = GetLastError();
	if (error == ERROR_OPERATION_ABORTED)
	{
		Log("NamedPipeInstance.Connect.Aborted", Severity::Verbose) << "ConnectNamedPipe returned ERROR_OPERATION_ABORTED.";
		return IoResult::Aborted;
	}

	Log("NamedPipeInstance.Connect.UnknownError", Severity::Error)
		<< R"(ConnectNamedPipe failed with unexpected error. { "error": )" << error << R"( })";
	throw std::runtime_error("ConnectNamedPipe failed unexpectedly.");

	return IoResult::Error;
}
