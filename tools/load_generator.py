import asyncio, statistics, sys, time

import matplotlib
matplotlib.use("Agg")                      # write to file, no window
import matplotlib.pyplot as plt

HOST, PORT = "127.0.0.1", 8080
STEP, MAX_CLIENTS, ROUNDS = 100, 5000, 20


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


async def fan_out_latency(readers, writers, rounds=ROUNDS):
    """Client 0 sends; time until every other client has received it."""
    lat = []
    for k in range(rounds):
        payload = f"ping-{k}".encode()
        t0 = time.perf_counter()
        writers[0].write(payload)
        await writers[0].drain()
        await asyncio.gather(*(r.read(200) for r in readers[1:]))
        lat.append((time.perf_counter() - t0) * 1000)
    lat.sort()
    return statistics.median(lat), lat[int(len(lat) * 0.99)], lat[-1]


async def sweep():
    readers, writers, results = [], [], []
    for target in range(STEP, MAX_CLIENTS + 1, STEP):
        if not await add_clients(readers, writers, target - len(writers)):
            break
        p50, p99, mx = await fan_out_latency(readers, writers)
        n = len(writers)
        results.append((n, p50, p99, mx))
        print(f"n = {n:5d}  p50 = {p50:6.1f} ms  p99 = {p99:6.1f} ms  max = {mx:6.1f} ms")
    for w in writers:
        w.close()
    return results


def plot(results, out_path):
    n, p50, p99, mx = zip(*results)
    plt.figure(figsize=(8, 5))
    plt.plot(n, p50, marker="o", label="p50")
    plt.plot(n, p99, marker="s", label="p99")
    plt.plot(n, mx, marker="^", label="max", alpha=0.6)
    plt.xlabel("connected clients")
    plt.ylabel("fan-out latency (ms)")
    plt.title("Broadcast fan-out latency vs. client count")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    print(f"saved {out_path}")


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "v1_traffic.png"
    results = asyncio.run(sweep())
    if results:
        plot(results, out)