# Switches an imagexex-built XEX2 from devkit to retail encryption, like `xextool -m r`.
# Only the 16 byte file key in the security info changes: it gets re-encrypted with the retail key instead of the
# devkit key. The image data stays encrypted with the same file key.
param([Parameter(Mandatory)][string]$Path)
$ErrorActionPreference = 'Stop'

$DevkitKey = [byte[]]::new(16)
$RetailKey = [byte[]](0x20,0xB1,0x85,0xA5,0x9D,0x28,0xFD,0xC3,0x40,0x58,0x3F,0xBB,0x08,0x96,0xBF,0x91)

function BE32([byte[]]$b, [int]$o) { ([uint32]$b[$o] -shl 24) -bor ([uint32]$b[$o+1] -shl 16) -bor ([uint32]$b[$o+2] -shl 8) -bor $b[$o+3] }

function Aes([byte[]]$key, [string]$mode, [byte[]]$data, [bool]$encrypt) {
	$aes = [System.Security.Cryptography.Aes]::Create()
	$aes.Key = $key; $aes.IV = [byte[]]::new(16); $aes.Padding = 'None'; $aes.Mode = $mode
	$t = if ($encrypt) { $aes.CreateEncryptor() } else { $aes.CreateDecryptor() }
	$t.TransformFinalBlock($data, 0, $data.Length)
}

# Offset of the file format info (optional header 0x3FF): encryption type, compression type, first block
function Get-FormatInfo([byte[]]$xex) {
	$count = BE32 $xex 0x14
	for ($i = 0; $i -lt $count; $i++) {
		if ((BE32 $xex (0x18 + $i * 8)) -eq 0x3FF) { return [int](BE32 $xex (0x1C + $i * 8)) }
	}
	throw "No file format info header"
}

# Checks the file key against the image: a compressed image's first block must match the SHA-1 in the header,
# an uncompressed one must start with "MZ"
function Test-FileKey([byte[]]$xex, [byte[]]$fileKey) {
	$peOffset = [int](BE32 $xex 0x8)
	$info = Get-FormatInfo $xex
	$compression = ([int]$xex[$info + 6] -shl 8) -bor $xex[$info + 7]
	if ($compression -eq 2) {
		$blockSize = [int](BE32 $xex ($info + 12))
		$hash = [byte[]]$xex[($info + 16)..($info + 35)]
		$plain = Aes $fileKey 'CBC' ([byte[]]$xex[$peOffset..($peOffset + $blockSize - 1)]) $false
		$actual = [System.Security.Cryptography.SHA1]::Create().ComputeHash($plain)
		return [System.Linq.Enumerable]::SequenceEqual($actual, $hash)
	}
	$plain = Aes $fileKey 'CBC' ([byte[]]$xex[$peOffset..($peOffset + 15)]) $false
	$plain[0] -eq 0x4D -and $plain[1] -eq 0x5A
}

$xex = [System.IO.File]::ReadAllBytes($Path)
if ([System.Text.Encoding]::ASCII.GetString($xex, 0, 4) -ne 'XEX2') { throw "$Path is not an XEX2 file" }
$keyOffset = (BE32 $xex 0x10) + 0x150
$wrapped = [byte[]]$xex[$keyOffset..($keyOffset + 15)]

$fileKey = Aes $RetailKey 'ECB' $wrapped $false
if (Test-FileKey $xex $fileKey) { "Already retail: $Path"; exit 0 }

$fileKey = Aes $DevkitKey 'ECB' $wrapped $false
if (-not (Test-FileKey $xex $fileKey)) { throw "File key is neither devkit nor retail encrypted, or the image is compressed" }

$rewrapped = Aes $RetailKey 'ECB' $fileKey $true
[Array]::Copy($rewrapped, 0, $xex, $keyOffset, 16)

$check = Aes $RetailKey 'ECB' $rewrapped $false
if (-not (Test-FileKey $xex $check)) { throw "Verification after re-encryption failed" }

[System.IO.File]::WriteAllBytes($Path, $xex)
"Converted to retail encryption: $Path"
