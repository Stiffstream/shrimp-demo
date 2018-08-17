FROM ubuntu:18.04 as shrimpdemo-build

# Prepare build environment
RUN apt-get update && \
    apt-get -qq -y install gcc g++ ruby \
    cmake autoconf curl wget libpcre3-dev pkg-config \
    libjpeg-dev libpng-dev libgif-dev \
    libtool
RUN gem install Mxx_ru

ARG NASM_VERSION=2.13.03
RUN echo "*** Installing NASM-"${NASM-VERSION} \
    && cd /tmp \
    && curl -s -O -L https://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}/nasm-${NASM_VERSION}.tar.gz \
    && tar xzf nasm-${NASM_VERSION}.tar.gz \
    && rm /tmp/nasm-${NASM_VERSION}.tar.gz \
    && cd /tmp/nasm-${NASM_VERSION} \
    && sh configure \
    && make all -j4 \
    && make install \
    && rm -rf /tmp/nasm-${NASM_VERSION}

ARG x265_VERSION=2.8
RUN echo "*** Installing x265-"${x265_VERSION} \
    && cd /tmp \
    && curl -s -O -L http://ftp.videolan.org/pub/videolan/x265/x265_${x265_VERSION}.tar.gz \
    && tar xzf x265_${x265_VERSION}.tar.gz \
    && rm /tmp/x265_${x265_VERSION}.tar.gz \
    && cd /tmp/x265_${x265_VERSION}/build/linux \
    && cmake ../../source \
    && make -j4 \
    && make install \
    && rm -rf /tmp/x265_${x265_VERSION}

ARG libde265_VERSION=1.0.3
ARG libde265_ARCHIVE=v${libde265_VERSION}
RUN echo "*** Installing libde265-"${libde265_VERSION} \
    && cd /tmp \
    && curl -s -O -L https://github.com/strukturag/libde265/archive/${libde265_ARCHIVE}.zip \
    && unzip ${libde265_ARCHIVE}.zip \
    && rm /tmp/${libde265_ARCHIVE}.zip \
    && cd /tmp/libde265-${libde265_VERSION} \
    && ./autogen.sh \
    && ./configure \
    && make -j4 \
    && make install \
    && rm -rf /tmp/libde265-${libde265_VERSION}

ARG libheif_VERSION=1.3.2
RUN echo "*** Installing libheif-"${libheif_VERSION} \
    && cd /tmp \
    && curl -s -O -L https://github.com/strukturag/libheif/archive/v${libheif_VERSION}.zip \
    && unzip v${libheif_VERSION}.zip \
    && rm /tmp/v${libheif_VERSION}.zip \
    && cd /tmp/libheif-${libheif_VERSION} \
    && ./autogen.sh \
    && ./configure \
    && make -j4 \
    && make install \
    && rm -rf /tmp/libheif-${libheif_VERSION}

ARG libwebp_VERSION=1.0.0
RUN echo "*** Installing libwebp-"${libwebp_VERSION} \
    && cd /tmp \
    && curl -s -O -L https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-${libwebp_VERSION}.tar.gz \
    && tar xzf libwebp-${libwebp_VERSION}.tar.gz \
    && rm /tmp/libwebp-${libwebp_VERSION}.tar.gz \
    && cd /tmp/libwebp-${libwebp_VERSION} \
    && ./configure \
    && make -j4 \
    && make install \
    && rm -rf /tmp/libwebp-${libwebp_VERSION}

ARG ImageMagick_VERSION=7.0.7
ARG ImageMagick_PATCH=39
ARG ImageMagick_FullVersion=${ImageMagick_VERSION}-${ImageMagick_PATCH}
RUN echo "*** Installing ImageMagick-"${ImageMagick_FullVersion} \
    && cd /tmp \
    && curl -s -O -L https://github.com/ImageMagick/ImageMagick/archive/${ImageMagick_FullVersion}.tar.gz \
    && tar xzf ${ImageMagick_FullVersion}.tar.gz \
    && rm /tmp/${ImageMagick_FullVersion}.tar.gz \
    && cd /tmp/ImageMagick-${ImageMagick_FullVersion} \
    && ./configure \
    && make -j4 \
    && make install \
    && ldconfig \
    && rm -rf /tmp/ImageMagick-${ImageMagick_FullVersion} \
    && convert -version

RUN mkdir /tmp/shrimp-dev
COPY externals.rb /tmp/shrimp-dev
COPY dev /tmp/shrimp-dev/dev

RUN echo "*** Building Shrimp ***" \
    && cd /tmp/shrimp-dev \
    && mxxruexternals \
	 && cd dev \
	 && MXX_RU_CPP_TOOLSET=gcc_linux ruby shrimp/app/prj.rb --mxx-cpp-release \
	 && cp target/release/shrimp.app /root \
	 && cd /root \
	 && rm -rf /tmp/shrimp-dev

FROM ubuntu:18.04 as shrimpdemo

ARG ImageMagick_VERSION=7.0.7

RUN apt-get update \
    && apt-get -qq -y install libjpeg8 libpng16-16 libgif7 libgomp1 

COPY --from=shrimpdemo-build /root/shrimp.app /root

COPY --from=shrimpdemo-build /usr/local/etc/ImageMagick-7 /usr/local/etc/ImageMagick-7

COPY --from=shrimpdemo-build /usr/local/lib/ImageMagick-${ImageMagick_VERSION} /usr/local/lib/ImageMagick-${ImageMagick_VERSION}

COPY --from=shrimpdemo-build /usr/local/lib/lib*.so /usr/local/lib/

RUN ldconfig


# Shrimp runs on port 80.
EXPOSE 80
WORKDIR /root

# Start Shrimp server
CMD ~/shrimp.app -a 0.0.0.0 -p 80 -i ~/images

