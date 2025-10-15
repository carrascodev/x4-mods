FROM heroiclabs/nakama-pluginbuilder:3.26.0 AS builder

ENV GO111MODULE=on
ENV CGO_ENABLED=1
ENV GOPRIVATE="hiro/guides"

WORKDIR /backend
COPY . .

RUN go build --trimpath --buildmode=plugin -o ./backend.so
RUN go install github.com/go-delve/delve/cmd/dlv@latest

FROM heroiclabs/nakama:3.26.0

COPY --from=builder /go/bin/dlv /nakama
COPY --from=builder /backend/backend.so /nakama/data/modules
COPY --from=builder /backend/local.yml /nakama/data/
