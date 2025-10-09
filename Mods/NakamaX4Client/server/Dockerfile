FROM heroiclabs/nakama-pluginbuilder:latest AS builder

ENV GO111MODULE on
ENV CGO_ENABLED 1
ENV GOPRIVATE "hiro/guides"

WORKDIR /backend
COPY . .

RUN go build --trimpath --gcflags "all=-N -l" --buildmode=plugin -o ./backend.so
RUN go install github.com/go-delve/delve/cmd/dlv@latest

FROM heroiclabs/nakama-dsym:latest

COPY --from=builder /go/bin/dlv /nakama
COPY --from=builder /backend/backend.so /nakama/data/modules
COPY --from=builder /backend/local.yml /nakama/data/
