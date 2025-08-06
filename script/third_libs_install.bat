@echo off

echo "Creating Third Library..."

set SOURCE_DIR=./source
set INSTALL_DIR=D:/library/install

set ASIO_DIR=asio-1.30.2
set LUA_DIR=lua-5.4.7

set ZLIB_INC_DIR=%INSTALL_DIR%/zlib/include
set ZLIB_LIB_DEBUG=%INSTALL_DIR%/zlib/lib/zlibd.lib
set ZLIB_LIB_RELEASE=%INSTALL_DIR%/zlib/lib/zlib.lib

set CMAKE=cmake

set TARGET_LIST=

if NOT exist %SOURCE_DIR% (
    mkdir %SOURCE_DIR%
)

if NOT exist %INSTALL_DIR% (
    mkdir %INSTALL_DIR%
)

cd %SOURCE_DIR%

@REM spdlog
@REM git clone -b v1.15.3 https://github.com/gabime/spdlog.git

@REM cd ./spdlog
@REM call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -DCMAKE_DEBUG_POSTFIX='d' -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/spdlog -DSPDLOG_BUILD_EXAMPLE=OFF -DSPDLOG_BUILD_SHARED=ON

@REM set TARGET_LIST=%TARGET_LIST% spdlog
@REM cd ../

@REM asio
@REM if exist %ASIO_DIR% (
@REM     cd %ASIO_DIR%
@REM     call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/asio
    
@REM     set TARGET_LIST=%TARGET_LIST% %ASIO_DIR%
@REM     cd ../
@REM )


@REM lua
@REM if exist %LUA_DIR% (
@REM     cd %LUA_DIR%
@REM     call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/lua
    
@REM     set TARGET_LIST=%TARGET_LIST% %LUA_DIR%
@REM     cd ../
@REM )

@REM json
@REM git clone -b v3.12.0 https://github.com/nlohmann/json.git

@REM cd ./json
@REM call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/nlohmann_json -DJSON_BuildTests=OFF -DBUILD_TESTING=OFF

@REM set TARGET_LIST=%TARGET_LIST% json
@REM cd ../

@REM YAML
@REM git clone -b 0.8.0 https://github.com/jbeder/yaml-cpp.git

@REM cd ./yaml-cpp
@REM call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/YAML_CPP -DYAML_BUILD_SHARED_LIBS=ON -DYAML_CPP_BUILD_TESTS=OFF -DBUILD_TESTING=OFF

@REM set TARGET_LIST=%TARGET_LIST% yaml-cpp
@REM cd ../

@REM zlib
@REM git clone -b v1.3.1 https://github.com/madler/zlib.git

@REM cd ./zlib
@REM call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/zlib -DCMAKE_DEBUG_POSTFIX='d' -DINSTALL_BIN_DIR=%INSTALL_DIR%/zlib/bin -DINSTALL_INC_DIR=%INSTALL_DIR%/zlib/include -DINSTALL_LIB_DIR=%INSTALL_DIR%/zlib/lib -DINSTALL_MAN_DIR=%INSTALL_DIR%/zlib/share/man -DINSTALL_PKGCONFIG_DIR=%INSTALL_DIR%/zlib/share/pkgconfig

@REM set TARGET_LIST=%TARGET_LIST% zlib
@REM cd ../

@REM mimalloc
git clone -b v3.1.5 https://github.com/microsoft/mimalloc.git
cd ./mimalloc
call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/libmimalloc -DMI_BUILD_TEST=OFF -DCMAKE_DEBUG_POSTFIX='d'

set TARGET_LIST=%TARGET_LIST% libmimalloc
cd ../

@REM hiredis
@REM git clone -b v1.3.0 https://github.com/redis/hiredis.git

@REM cd ./hiredis
@REM call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/hiredis -DCMAKE_DEBUG_POSTFIX='d' -DDISABLE_TESTS=ON -DBUILD_SHARED_LIBS=ON

@REM set TARGET_LIST=%TARGET_LIST% hiredis
@REM cd ../


for %%i in (%TARGET_LIST%) do (
    cd %%i/build
    cd

    call %CMAKE% --build . --target install --config=Debug
    call %CMAKE% --build . --target install --config=Release

    cd ../../
)
set TARGET_LIST=

@REM redis-plus-plus
@REM git clone -b 1.3.14 https://github.com/sewenew/redis-plus-plus.git
cd ./redis-plus-plus
call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -Dhiredis_DIR=%INSTALL_DIR%/hiredis/share/hiredis -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/redis++ -DCMAKE_DEBUG_POSTFIX='d' -DREDIS_PLUS_PLUS_BUILD_TEST=OFF -DREDIS_PLUS_PLUS_BUILD_SHARED=ON

set TARGET_LIST=%TARGET_LIST% redis-plus-plus
cd ../

@REM protobuf
@REM git clone -b v30.2 https://github.com/protocolbuffers/protobuf.git

@REM cd ./protobuf
@REM call %CMAKE% . -G "Visual Studio 17 2022" -B ./build -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%/protobuf -DCMAKE_DEBUG_POSTFIX='d' -DCMAKE_CXX_STANDARD=20 -DBUILD_TESTING=OFF -Dprotobuf_BUILD_LIBPROTOC=ON -Dprotobuf_BUILD_SHARED_LIBS=ON -Dprotobuf_BUILD_TESTS=OFF -DZLIB_INCLUDE_DIR=%ZLIB_INC_DIR% -DZLIB_LIBRARY_DEBUG=%ZLIB_LIB_DEBUG% -DZLIB_LIBRARY_RELEASE=%ZLIB_LIB_RELEASE%

@REM set TARGET_LIST=%TARGET_LIST% protobuf

@REM cd ../

for %%i in (%TARGET_LIST%) do (
    cd %%i/build
    cd

    call %CMAKE% --build . --target install --config=Debug
    call %CMAKE% --build . --target install --config=Release

    cd ../../
)

cd ../

pause