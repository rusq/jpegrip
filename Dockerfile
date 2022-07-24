FROM frolvlad/alpine-gcc as build

WORKDIR /build

COPY . .

RUN apk add --no-cache make

# (1) silkeh/clang
# RUN ln -s `which clang` /usr/bin/cc
# (1) END

RUN make

FROM alpine:3.16

COPY --from=build /build/jpegrip /usr/local/bin

ENTRYPOINT ["jpegrip"]
