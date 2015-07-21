if ($Global:GitStatusCacheClientPipe -eq $null)
{
	$Global:GitStatusCacheClientPipe = new-object System.IO.Pipes.NamedPipeClientStream '.','GitStatusCache','InOut','WriteThrough'
	$Global:GitStatusCacheClientPipe.Connect(50)
	$Global:GitStatusCacheClientPipe.ReadMode = 'Message'
}

$timer = [system.diagnostics.stopwatch]::StartNew()

$request = new-object psobject -property @{ Version = 1; Path = $PSScriptRoot } | ConvertTo-Json -Compress
$encoding = [System.Text.Encoding]::Unicode
$requestBuffer = $encoding.GetBytes($request)
$Global:GitStatusCacheClientPipe.Write($requestBuffer, 0, $requestBuffer.Length)

$responseBuffer = new-object byte[] $Global:GitStatusCacheClientPipe.InBufferSize
$bytesRead = $Global:GitStatusCacheClientPipe.Read($responseBuffer, 0, $responseBuffer.Length)
$response = $encoding.GetString($responseBuffer, 0, $bytesRead)

$timer.Stop()
Write-Host "Completed request after $($timer.ElapsedMilliseconds) ms: $response"
