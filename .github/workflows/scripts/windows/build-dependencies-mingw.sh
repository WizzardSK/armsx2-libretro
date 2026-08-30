#!/usr/bin/env bash
#
# Builds the dependencies of the libretro core for Windows with the MinGW-w64
# cross toolchain, static, into a local prefix. The buildbot's Windows image is
# a cross image with no packages for any of these, and the core has to carry
# them anyway.
#
# Usage: build-dependencies-mingw.sh <install-prefix>

set -e

if [ "$#" -ne 1 ]; then
	echo "Syntax: $0 <install-prefix>"
	exit 1
fi

PREFIX=$(realpath "$1")
NPROCS="$(getconf _NPROCESSORS_ONLN)"
SCRIPTDIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# Versions follow the Linux recipe (build-dependencies-runner.sh) where they
# overlap; zlib, jpeg and the DirectX headers are system packages there and
# only exist here.
FREETYPE=VER-2-14-1
LIBPNG=v1.6.51
LIBWEBP=v1.6.0
SDL=release-3.2.26
LZ4=v1.10.0
ZSTD=v1.5.7
PLUTOVG=v1.3.2
PLUTOSVG=v0.0.7
ZLIB=v1.3.1
JPEGTURBO=3.1.4.1
SHADERC=v2025.4
SHADERC_GLSLANG=7a47e2531cb334982b2a2dd8513dca0a3de4373d

TOOLCHAIN=(
	"-DCMAKE_TOOLCHAIN_FILE=$SCRIPTDIR/mingw-toolchain.cmake"
	"-DCMAKE_INSTALL_PREFIX=$PREFIX"
	"-DCMAKE_PREFIX_PATH=$PREFIX"
	"-DCMAKE_FIND_ROOT_PATH=$PREFIX;/usr/x86_64-w64-mingw32"
	"-DCMAKE_BUILD_TYPE=Release"
	"-DBUILD_SHARED_LIBS=OFF"
)

mkdir -p deps-build
cd deps-build

clone() {
	[ -d "$2" ] || git clone --depth 1 --branch "$3" --recursive "$1" "$2"
}

build() {
	local src="$1" bld="$2"
	shift 2
	cmake -S "$src" -B "$bld" -G Ninja "${TOOLCHAIN[@]}" "$@"
	cmake --build "$bld" --parallel "$NPROCS"
	cmake --install "$bld"
}

clone https://github.com/madler/zlib zlib "$ZLIB"
build zlib zlib/build -DZLIB_BUILD_SHARED=OFF -DZLIB_BUILD_STATIC=ON \
	-DZLIB_BUILD_TESTING=OFF -DZLIB_BUILD_MINIZIP=OFF -DZLIB_BUILD_EXAMPLES=OFF
# Some zlib revisions install the static archive as libzs.a, which FindZLIB
# does not look for.
if [ -f "$PREFIX/lib/libzs.a" ] && [ ! -f "$PREFIX/lib/libz.a" ]; then
	cp "$PREFIX/lib/libzs.a" "$PREFIX/lib/libz.a"
fi

clone https://github.com/pnggroup/libpng libpng "$LIBPNG"
build libpng libpng/build -DPNG_SHARED=OFF -DPNG_STATIC=ON -DPNG_TESTS=OFF \
	-DPNG_TOOLS=OFF -DPNG_FRAMEWORK=OFF

clone https://github.com/libjpeg-turbo/libjpeg-turbo libjpeg-turbo "$JPEGTURBO"
build libjpeg-turbo libjpeg-turbo/build -DENABLE_SHARED=OFF -DENABLE_STATIC=ON \
	-DWITH_TURBOJPEG=OFF

clone https://github.com/facebook/zstd zstd "$ZSTD"
build zstd/build/cmake zstd/b -DZSTD_BUILD_SHARED=OFF -DZSTD_BUILD_STATIC=ON \
	-DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_TESTS=OFF

clone https://github.com/lz4/lz4 lz4 "$LZ4"
build lz4/build/cmake lz4/b -DLZ4_BUILD_CLI=OFF -DLZ4_BUILD_LEGACY_LZ4C=OFF

clone https://github.com/webmproject/libwebp libwebp "$LIBWEBP"
# SIMD off: the AVX2 translation unit compiles without -mavx2 under this cross
# build, so _mm256_cvtsi256_si32 and friends are left as calls that nothing
# defines. The core only uses webp for texture replacements, so the scalar
# paths are no loss worth chasing this for.
build libwebp libwebp/build -DWEBP_ENABLE_SIMD=OFF -DWEBP_BUILD_ANIM_UTILS=OFF -DWEBP_BUILD_CWEBP=OFF \
	-DWEBP_BUILD_DWEBP=OFF -DWEBP_BUILD_GIF2WEBP=OFF -DWEBP_BUILD_IMG2WEBP=OFF \
	-DWEBP_BUILD_VWEBP=OFF -DWEBP_BUILD_WEBPINFO=OFF -DWEBP_BUILD_WEBPMUX=OFF \
	-DWEBP_BUILD_EXTRAS=OFF

clone https://github.com/libsdl-org/SDL sdl3 "$SDL"
build sdl3 sdl3/build -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF

clone https://github.com/freetype/freetype freetype "$FREETYPE"
build freetype freetype/build -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON \
	-DFT_DISABLE_PNG=ON -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON

clone https://github.com/sammycage/plutovg plutovg "$PLUTOVG"
build plutovg plutovg/build -DPLUTOVG_BUILD_EXAMPLES=OFF

clone https://github.com/sammycage/plutosvg plutosvg "$PLUTOSVG"
build plutosvg plutosvg/build -DPLUTOSVG_ENABLE_FREETYPE=ON -DPLUTOSVG_BUILD_EXAMPLES=OFF

# No DirectX-Headers: the Direct3D renderers are not built here (USE_D3D=OFF).
# They cannot be - GSDevice11.h needs WIL, which needs Windows SDK headers
# (WeakReference.h) that mingw-w64 does not ship - and the core never uses them
# anyway, since the frontend hands it a Vulkan device. DirectX-Headers itself
# also does not compile with mingw's gcc: its d3d12.h uses SAL annotations
# (_In_opt_count_) that the bundled wsl/stubs do not cover.

clone https://github.com/google/shaderc shaderc "$SHADERC"
(cd shaderc && python3 utils/git-sync-deps)
if [ "$(git -C shaderc/third_party/glslang rev-parse HEAD)" != "$SHADERC_GLSLANG" ]; then
	git -C shaderc/third_party/glslang fetch --depth 1 origin "$SHADERC_GLSLANG"
	git -C shaderc/third_party/glslang checkout --detach FETCH_HEAD
fi
cmake -S shaderc -B shaderc/b -G Ninja "${TOOLCHAIN[@]}" \
	-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
	-DSHADERC_SKIP_TESTS=ON -DSHADERC_SKIP_EXAMPLES=ON -DSHADERC_SKIP_COPYRIGHT_CHECK=ON
cmake --build shaderc/b --parallel "$NPROCS" --target shaderc_combined
mkdir -p "$PREFIX/lib" "$PREFIX/include"
cp shaderc/b/libshaderc/libshaderc_combined.a "$PREFIX/lib/"
cp -r shaderc/libshaderc/include/shaderc "$PREFIX/include/"

echo "Windows (MinGW) dependencies installed to $PREFIX"
