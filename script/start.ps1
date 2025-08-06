$libraryDir = "D:\\library\install"
$serverDir = "$PSScriptRoot\msvc_build\Debug"

$CopyList = @(
    @{ Name = "spdlog"; BinPath = "spdlog\bin"; DllName = "spdlogd.dll" },
    @{ Name = "protobuf"; BinPath = "protobuf\bin"; DllName = "abseil_dlld.dll" },
    @{ Name = "protobuf"; BinPath = "protobuf\bin"; DllName = "libprotobufd.dll" },
    @{ Name = "protobuf"; BinPath = "protobuf\bin"; DllName = "libprotobuf-lited.dll" },
    @{ Name = "protobuf"; BinPath = "protobuf\bin"; DllName = "libprotocd.dll" },
    @{ Name = "protobuf"; BinPath = "protobuf\bin"; DllName = "libutf8_ranged.dll" },
    @{ Name = "protobuf"; BinPath = "protobuf\bin"; DllName = "libutf8_validityd.dll" },
    @{ Name = "zlib"; BinPath = "protobuf\bin"; DllName = "zlibd.dll" },
    @{ Name = "yaml-cpp"; BinPath = "YAML_CPP\bin"; DllName = "yaml-cppd.dll" },
    @{ Name = "mysql"; BinPath = "mysql-connector-c++-9.3.0-winx64-debug\lib64"; DllName = "libcrypto-3-x64.dll" },
    @{ Name = "mysql"; BinPath = "mysql-connector-c++-9.3.0-winx64-debug\lib64"; DllName = "libssl-3-x64.dll" },
    @{ Name = "mysql"; BinPath = "mysql-connector-c++-9.3.0-winx64-debug\lib64\debug"; DllName = "mysqlcppconn-10-vs14.dll" },
    @{ Name = "mysql"; BinPath = "mysql-connector-c++-9.3.0-winx64-debug\lib64\debug"; DllName = "mysqlcppconnx-2-vs14.dll" }
)

if (-not (Test-Path -Path $serverDir)) {
    New-Item -ItemType Directory -Path $serverDir | Out-Null
    Write-Host "Create Server Directory: $serverDir" -ForegroundColor Green
}

foreach ($dll in $CopyList) {
    $sourcePath = Join-Path -Path $libraryDir -ChildPath (Join-Path -Path $dll.BinPath -ChildPath $dll.DllName)
    $destPath = Join-Path -Path $serverDir -ChildPath $dll.DllName
    
    try {
        # 检查源文件是否存在
        if (Test-Path -Path $sourcePath) {
            # 获取源文件和目标文件的最后修改时间
            $sourceLastModified = (Get-Item -Path $sourcePath).LastWriteTime
            $destLastModified = $null
            
            if (Test-Path -Path $destPath) {
                $destLastModified = (Get-Item -Path $destPath).LastWriteTime
            }
            
            # 如果源文件比目标文件新，或者目标文件不存在，则执行拷贝
            if ($null -eq $destLastModified -or $sourceLastModified -gt $destLastModified) {
                Copy-Item -Path $sourcePath -Destination $destPath -Force
                Write-Host ("$($dll.DllName)".PadRight(20) + "Copied") -ForegroundColor Green
            } else {
                Write-Host "$($dll.DllName) Is The Lastest, Skipped" -ForegroundColor Yellow
            }
        } else {
            Write-Host "Can Not Found $sourcePath" -ForegroundColor Red
        }
    }
    catch {
        Write-Host "Exception Occurred While Copying $($dll.DllName): $_" -ForegroundColor Red
    }
}

Copy-Item "$PSScriptRoot\msvc_build\login\Debug\login.dll" -Destination $serverDir

$agentDir = Join-Path -Path $serverDir -ChildPath "agent"
$serviceDir = Join-Path -Path $serverDir -ChildPath "service"

if (-not (Test-Path -Path $agentDir)) {
    New-Item -ItemType Directory -Path $agentDir | Out-Null
    Write-Host "Create Agent Directory: $agentDir" -ForegroundColor Green
}

if (-not (Test-Path -Path $serviceDir)) {
    New-Item -ItemType Directory -Path $serviceDir | Out-Null
    Write-Host "Create Service Directory: $serviceDir" -ForegroundColor Green
}


Copy-Item -Path "$PSScriptRoot\msvc_build\service\agent\Debug\agent.dll" -Destination "$PSScriptRoot\msvc_build\Debug\agent"


$serviceRoot = "$PSScriptRoot\msvc_build\service"
$ServiceList = @(
    @{ Name = "gameworld" }
)

foreach ($dll in $ServiceList) {
    $sourcePath = Join-Path -Path $serviceRoot -ChildPath (Join-Path -Path "$($dll.Name)\Debug" -ChildPath "$($dll.Name).dll")
    $destPath = Join-Path -Path $serviceDir -ChildPath "$($dll.Name).dll"
    
    try {
        # 检查源文件是否存在
        if (Test-Path -Path $sourcePath) {
            # 获取源文件和目标文件的最后修改时间
            $sourceLastModified = (Get-Item -Path $sourcePath).LastWriteTime
            $destLastModified = $null
            
            if (Test-Path -Path $destPath) {
                $destLastModified = (Get-Item -Path $destPath).LastWriteTime
            }
            
            # 如果源文件比目标文件新，或者目标文件不存在，则执行拷贝
            if ($null -eq $destLastModified -or $sourceLastModified -gt $destLastModified) {
                Copy-Item -Path $sourcePath -Destination $destPath -Force
                Write-Host ("$($dll.Name).dll".PadRight(20) + "Copied") -ForegroundColor Green
            } else {
                Write-Host "$($dll.Name).dll Is The Lastest, Skipped" -ForegroundColor Yellow
            }
        } else {
            Write-Host "Can Not Found $sourcePath" -ForegroundColor Red
        }
    }
    catch {
        Write-Host "Exception Occurred While Copying $($dll.Name): $_" -ForegroundColor Red
    }
}

Set-Location .\msvc_build\Debug\
.\uranus.exe