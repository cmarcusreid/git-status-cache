function Dispose-Pipe
{
	$Global:GitStatusCacheClientPipe.Dispose()
	$Global:GitStatusCacheClientPipe = $null
}

function Initialize-Pipe
{
	if ($Global:GitStatusCacheClientPipe -ne $null -and -not $Global:GitStatusCacheClientPipe.IsConnected)
	{
		Dispose-Pipe
	}

	if ($Global:GitStatusCacheClientPipe -eq $null)
	{
		$Global:GitStatusCacheClientPipe = new-object System.IO.Pipes.NamedPipeClientStream '.','GitStatusCache','InOut','WriteThrough'
		$Global:GitStatusCacheClientPipe.Connect(50)
		$Global:GitStatusCacheClientPipe.ReadMode = 'Message'
	}
}

$timer = [system.diagnostics.stopwatch]::StartNew()

Initialize-Pipe

$remainingRetries = 1
while ($remainingRetries -ge 0)
{
	$request = new-object psobject -property @{ Version = 1; Path = (Get-Location).Path } | ConvertTo-Json -Compress
	$encoding = [System.Text.Encoding]::Unicode
	$requestBuffer = $encoding.GetBytes($request)

	$wasPipeBroken = $false
	try
	{
		$Global:GitStatusCacheClientPipe.Write($requestBuffer, 0, $requestBuffer.Length)
	}
	catch [system.io.ioexception]
	{
		Dispose-Pipe
		Initialize-Pipe
		--$remainingRetries
		$wasPipeBroken = $true
	}

	if (-not $wasPipeBroken)
	{
		$responseBuffer = new-object byte[] $Global:GitStatusCacheClientPipe.InBufferSize
		$bytesRead = $Global:GitStatusCacheClientPipe.Read($responseBuffer, 0, $responseBuffer.Length)
		$response = $encoding.GetString($responseBuffer, 0, $bytesRead)
		$remainingRetries = -1
	}
}

$timer.Stop()
Write-Host "Completed request after $($timer.ElapsedMilliseconds) ms: $response"
