lua_bin := "pdbunnymarklua.pdx"
sdk := env("PLAYDATE_SDK_PATH")
arm_gcc := "/usr/local/playdate/gcc-arm-none-eabi-9-2019-q4-major/bin"
c_bin := "pdbunnymarkc.pdx"

# List available recipes
default:
    @just --list

[working-directory("lua")]
lua-build:
    pdc source {{ lua_bin }}

[working-directory("lua")]
lua-run: lua-build
    open {{ lua_bin }}

[working-directory("lua")]
lua-clean:
    rm -rf {{ lua_bin }}

[working-directory("c")]
c-build:
    PATH="{{ arm_gcc }}:$PATH" make PLAYDATE_SDK_PATH={{ sdk }} EXTRA_CFLAGS="-Wimplicit-fallthrough"

[working-directory("c")]
c-run: c-build
    open {{ c_bin }}

[working-directory("c")]
c-clean:
    rm -rf {{ c_bin }}
