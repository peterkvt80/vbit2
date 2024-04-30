# make the buil container
FROM alpine as build-env
RUN apk add --no-cache build-base
WORKDIR /vbit2
# copy everything from source dir into build container
COPY . .
RUN make -j8

# make the runtime container
FROM alpine
RUN apk update && apk add --no-cache libstdc++ libgcc socat
COPY --from=build-env /vbit2/vbit2 /vbit2/vbit2
WORKDIR /vbit2
CMD ["/vbit2/vbit2"]

# build:
# docker build -t vbit2 .

# run:
# docker run -it --mount type=bind,source="/path/to/pages,target=/vbit2/pages,readonly vbit2
