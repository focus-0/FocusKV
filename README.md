# FocusKV

A minimal key-value store in C++17 (~2,200 LOC).

- **Storage:** WAL + MemTable + SSTable flush + on-disk manifest + bloom filters
- **Network:** TCP text protocol (`SET`, `GET`, `DEL`)
- **Deployment:** Independent nodes (no replication or consensus)

Each node is a standalone server with its own on-disk database. You can run multiple nodes on different ports, but they do not share or replicate data.

## Build

```bash
bazel build //...
bazel test //:memtable_test //:wal_test //:sstable_test //:db_test //:server_test //:cluster_test
```

## Run a node

```bash
bazel run //:focuskv_node -- --port=7001 --data=/tmp/focuskv_n1
```

Run more nodes on different ports and data dirs if you want multiple independent servers.

## Client commands

```bash
nc localhost 7001
SET user:1 Ayush Lohumi
GET user:1
DEL user:1
```

### Responses

| Command | Success | Failure |
|---|---|---|
| SET | `+OK\r\n` | `-ERR ...\r\n` |
| GET | `$<len>\r\n<value>\r\n` | `-ERR key not found\r\n` |
| DEL | `:1\r\n` | `:0\r\n` |

Writes and reads go directly to that node's local database.

## Options

| Option | Default | Purpose |
|---|---|---|
| `write_buffer_size` | 4 MB | MemTable size before flush to SST |
| `wal_sync_every` | 32 | Group commit — fsync WAL every N writes |

Set `wal_sync_every = 1` for fsync on every write (safest, slowest). On close and flush, the WAL is always synced regardless of this setting.

## On-disk files (per node data dir)

| File | Purpose |
|---|---|
| `wal.log` | Write-ahead log |
| `MANIFEST` | SSTable list + sequence metadata |
| `*.sst` | Flushed sorted tables (includes a bloom filter) |

## Layout

```
src/storage/   DB, WAL, MemTable, SSTable, bloom filter, manifest
src/network/   TCP server
cmd/           focuskv_node binary
tests/         unit + integration tests
```

## Verification

| Test | What it proves |
|---|---|
| `memtable_test` | In-memory put/get/delete and overwrite |
| `wal_test` | WAL append + replay after restart |
| `sstable_test` | SST build, bloom filter, and point lookup |
| `db_test` | Full storage stack; memtable flush to SST |
| `server_test` | TCP `SET`/`GET`/`DEL` over socket |
| `cluster_test.IndependentNodesDoNotShareData` | Three nodes run independently; data on one is not visible on others |

## License

MIT
