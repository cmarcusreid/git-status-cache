#include "stdafx.h"
#include "NamedPipeServer.h"
#include "NamedPipeInstance.h"

void NamedPipeServer::WaitForClientRequest()
{
	Log("NamedPipeServer.WaitForClientRequest.Start", Severity::Verbose) << "Server thread started.";

	while (true)
	{
		RemoveClosedPipeInstances();

		Log("NamedPipeServer.WaitForClientRequest", Severity::Verbose) << "Creating named pipe instance and waiting for client.";
		auto pipe = std::make_unique<NamedPipeInstance>(m_onClientRequestCallback);
		auto connectResult = pipe->Connect();
		m_pipeInstances.emplace_back(std::move(pipe));

		if (connectResult != NamedPipeInstance::IoResult::Success)
		{
			Log("NamedPipeServer.WaitForClientRequest.Stop", Severity::Verbose) << "Server thread stopping.";
			break;
		}
	}
}

void NamedPipeServer::RemoveClosedPipeInstances()
{
	auto originalSize = m_pipeInstances.size();

	m_pipeInstances.erase(
		std::remove_if(
			m_pipeInstances.begin(),
			m_pipeInstances.end(),
			[](const std::unique_ptr<NamedPipeInstance>& pipeInstance) { return pipeInstance->IsClosed(); }),
		m_pipeInstances.end());

	auto newSize = m_pipeInstances.size();
	if (newSize != originalSize)
	{
		Log("NamedPipeServer.RemoveClosedPipeInstances", Severity::Spam)
			<< R"(Removed closed pipe instancs. { "instancesRemoved": )" << originalSize - newSize << " }";
	}
}

NamedPipeServer::NamedPipeServer(const OnClientRequestCallback& onClientRequestCallback)
	: m_onClientRequestCallback(onClientRequestCallback)
{
	Log("NamedPipeServer.StartingBackgroundThread", Severity::Spam) << "Attempting to start server thread.";
	m_serverThread = std::thread(&NamedPipeServer::WaitForClientRequest, this);
}

NamedPipeServer::~NamedPipeServer()
{
	Log("NamedPipeServer.ShutDown.StoppingBackgroundThread", Severity::Spam)
		<< R"(Shutting down server thread. { "threadId": 0x)" << std::hex << m_serverThread.get_id() << " }";
	::CancelSynchronousIo(m_serverThread.native_handle());
	m_serverThread.join();
}
