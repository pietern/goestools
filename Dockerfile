ARG version
FROM debian:${version} AS build-env

ENV TZ="Etc/UTC"
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get upgrade -y && apt-get install -y build-essential cmake git-core libairspy-dev libopencv-dev libproj-dev librtlsdr-dev zlib1g-dev pkgconf
WORKDIR /build
COPY . /goestools
RUN cmake /goestools
RUN make
RUN ls -al /build

FROM debian:${version} AS goesrecv
ENV TZ="Etc/UTC"
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get upgrade -y && apt-get install -y \
	librtlsdr0 \
	libairspy0 \
	&& rm -rf /var/lib/apt/lists/*
WORKDIR /goesrecv
COPY --from=build-env /build/src/goesrecv/goesrecv .
CMD ["./goesrecv","-c","/config/goesrecv.conf"]

FROM debian:${version} AS goesproc
ENV TZ="Etc/UTC"
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get upgrade -y && apt-get install -y \
	libopencv-dev \
	&& rm -rf /var/lib/apt/lists/*
WORKDIR /goesproc
COPY --from=build-env /build/src/goesproc/goesproc .
COPY --from=build-env /build/share/goesproc-goesr.conf ./config/goesproc.conf
CMD ["./goesproc","-c","/config/goesproc.conf","--out","/media/","-m","packet","--subscribe","tcp://127.0.0.1:5004"]
