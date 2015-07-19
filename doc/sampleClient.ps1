$pipe = new-object System.IO.Pipes.NamedPipeClientStream '.','GitStatusCache','InOut','WriteThrough'
$pipe.Connect(50)
$pipe.ReadMode = 'Message'

$request = $PSScriptRoot
$encoding = [System.Text.Encoding]::Unicode
$requestBuffer = $encoding.GetBytes($request)
$pipe.Write($requestBuffer, 0, $requestBuffer.Length)

$responseBuffer = new-object byte[] $pipe.InBufferSize
$bytesRead = $pipe.Read($responseBuffer, 0, $responseBuffer.Length)
$encoding.GetString($responseBuffer, 0, $bytesRead)

$pipe.Dispose()