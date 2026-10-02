import asyncio, statistics, sys, time

import matplotlib
matplotlib.use("Agg")                      # write to file, no window
import matplotlib.pyplot as plt

HOST, PORT = "127.0.0.1", 8080

# fanout: one client talks, N-1 listen.  Cost is O(N) sends per round.
# alltalk: every client talks at once.    Cost is O(N^2) sends per round,
#          which is where per-client write batching would show up.
MODES = {
    "fanout":  dict(step=100, max_clients=5000, rounds=20, out="v1_traffic.png"),
    "alltalk": dict(step=100, max_clients=1000, rounds=5,  out="v1_alltalk.png"),
}

MSG_LEN = 16          # fixed-size payloads so bytes received == messages received * MSG_LEN
ROUND_TIMEOUT = 120   # seconds before a round is declared stuck


async def add_clients(readers, writers, count):
    """Open `count` more connections, appending to the existing pools."""
    for _ in range(count):
        try:
            r, w = await asyncio.open_connection(HOST, PORT)
        except OSError as e:
            print(f"connect #{len(writers)} failed: {e}")
            return False
        readers.append(r)
        writers.append(w)
    await asyncio.sleep(0.5)               # let the server drain its accept backlog
    return True


async def fan_out_latency(readers, writers, rounds):
    """Client 0 sends; time until every other client has received it."""
    lat = []
    for k in range(rounds):
        payload = f"ping-{k}".encode()
        t0 = time.perf_counter()
        writers[0].write(payload)
        await writers[0].drain()
        await asyncio.gather(*(r.read(200) for r in readers[1:]))
        lat.append((time.perf_counter() - t0) * 1000)
    return lat


async def all_talk_latency(readers, writers, rounds):
    """Every client sends one message at the same instant; time until every
    client has received all N-1 messages from the others.

    The server does no framing, so several messages may arrive in one recv.
    Counting bytes with readexactly() is therefore the only reliable way to
    know a client has everything."""
    n = len(writers)
    expect = (n - 1) * MSG_LEN
    lat = []
    for k in range(rounds):
        t0 = time.perf_counter()
        for i, w in enumerate(writers):
            w.write(f"{i}:{k}".encode().ljust(MSG_LEN))
        await asyncio.gather(*(w.drain() for w in writers))
        await asyncio.gather(*(r.readexactly(expect) for r in readers))
        lat.append((time.perf_counter() - t0) * 1000)
    return lat


MEASURE = {"fanout": fan_out_latency, "alltalk": all_talk_latency}


def summarize(lat):
    lat.sort()
    return statistics.median(lat), lat[int(len(lat) * 0.99)], lat[-1]


async def sweep(mode):
    cfg = MODES[mode]
    measure = MEASURE[mode]
    readers, writers, results = [], [], []
    for target in range(cfg["step"], cfg["max_clients"] + 1, cfg["step"]):
        if not await add_clients(readers, writers, target - len(writers)):
            break
        n = len(writers)
        try:
            lat = await asyncio.wait_for(measure(readers, writers, cfg["rounds"]), ROUND_TIMEOUT)
        except asyncio.TimeoutError:
            print(f"n = {n:5d}  round did not finish within {ROUND_TIMEOUT}s, stopping")
            break
        except (asyncio.IncompleteReadError, ConnectionError) as e:
            print(f"n = {n:5d}  a client was dropped mid-round ({e}), stopping")
            break
        p50, p99, mx = summarize(lat)
        results.append((n, p50, p99, mx))
        line = f"n = {n:5d}  p50 = {p50:8.1f} ms  p99 = {p99:8.1f} ms  max = {mx:8.1f} ms"
        if mode == "alltalk":
            deliveries = n * (n - 1)
            line += f"  ({deliveries:>9,d} deliveries, {deliveries / (p50 / 1000):>12,.0f}/s)"
        print(line)
    for w in writers:
        w.close()
    return results


def plot(mode, results, out_path):
    n, p50, p99, mx = zip(*results)
    plt.figure(figsize=(8, 5))
    plt.plot(n, p50, marker="o", label="p50")
    plt.plot(n, p99, marker="s", label="p99")
    plt.plot(n, mx, marker="^", label="max", alpha=0.6)
    plt.xlabel("connected clients")
    if mode == "alltalk":
        plt.ylabel("all-talk round latency (ms)")
        plt.title("Everyone sends once: time until everyone has received all N-1 messages")
    else:
        plt.ylabel("fan-out latency (ms)")
        plt.title("Broadcast fan-out latency vs. client count")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    print(f"saved {out_path}")


if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "fanout"
    if mode not in MODES:
        sys.exit(f"usage: {sys.argv[0]} [{'|'.join(MODES)}] [out.png]")
    out = sys.argv[2] if len(sys.argv) > 2 else MODES[mode]["out"]
    results = asyncio.run(sweep(mode))
    if results:
        plot(mode, results, out)
