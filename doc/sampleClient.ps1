if ($pipe -eq $null)
{
	$pipe = new-object System.IO.Pipes.NamedPipeClientStream '.','GitStatusCache','InOut','WriteThrough'
	$pipe.Connect(50)
	$pipe.ReadMode = 'Message'
}

$timer = [system.diagnostics.stopwatch]::StartNew()

$request = new-object psobject -property @{ Version = 1; Path = $PSScriptRoot } | ConvertTo-Json -Compress
$encoding = [System.Text.Encoding]::Unicode
$requestBuffer = $encoding.GetBytes($request)
$pipe.Write($requestBuffer, 0, $requestBuffer.Length)

$responseBuffer = new-object byte[] $pipe.InBufferSize
$bytesRead = $pipe.Read($responseBuffer, 0, $responseBuffer.Length)
$response = $encoding.GetString($responseBuffer, 0, $bytesRead)

$timer.Stop()
Write-Host "Completed request after $($timer.ElapsedMilliseconds) ms: $response"
