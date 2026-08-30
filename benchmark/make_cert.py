"""Makes the self-signed certificate the TLS arm serves.

    python -m benchmark.make_cert --out benchmark/certs

Why a script rather than a committed pair of files. A private key in a repository is a
private key on every machine that ever cloned it, and the fact that this one only ever
protects a benchmark is not a distinction a secret scanner makes, nor one a reader
should have to take on trust. Generating it locally also means the certificate carries
the addresses of the machine that will actually serve it.

What the certificate is for, and what it is not. It exists so the handshake has
something to complete with. The generator does not verify it, which is stated in the
generator's own help text and belongs in the limitations of any measurement taken with
it: these numbers describe a TLS handshake without chain validation. A campaign that
wanted to include validation would need a certificate a real trust store accepts, and
would then be measuring the trust store as well.

Key type is the decision that matters most. RSA-2048 and P-256 do not cost the same:
the server's half of an RSA handshake is a private-key operation an order of magnitude
slower than the ECDSA one, and that cost lands on the server in both arms of the
comparison. It is chosen here, once, and recorded, rather than inherited from whatever
default the tool that made the file happened to have.
"""

from __future__ import annotations

import argparse
import ipaddress
import os
import shutil
import socket
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[1]

# ECDSA P-256 rather than RSA-2048. Both arms of the comparison pay the same private-key
# cost, so either would do for the A/B, but the absolute figures are more useful when
# the handshake is the one a current deployment would actually serve, and P-256 is what
# a modern default negotiates. Recorded here because a reader cannot recover it from a
# throughput number.
KEY_ALGORITHM = "EC"
KEY_CURVE = "prime256v1"
DAYS = 3650


def default_openssl() -> str:
    """An openssl this host can actually run.

    Windows ships none, but Git for Windows does, and every machine that can clone this
    repository has that. Looked up rather than assumed so the common case needs no flag:
    a run-book step that everyone has to remember is a step someone will forget, and the
    failure is a campaign that stops an hour in for want of a certificate.
    """
    found = shutil.which("openssl")
    if found:
        return found
    for candidate in (
        Path(os.environ.get("ProgramFiles", r"C:\Program Files")) / "Git" / "usr" / "bin" / "openssl.exe",
        Path(os.environ.get("ProgramW6432", r"C:\Program Files")) / "Git" / "usr" / "bin" / "openssl.exe",
    ):
        if candidate.exists():
            return str(candidate)
    return "openssl"


def local_addresses() -> list[str]:
    """Every IPv4 address this host answers on, so one certificate serves every campaign.

    The loopback campaign reaches the server at 127.0.0.1 and the campaign driven from
    WSL reaches it across the virtual switch at another address entirely. A certificate
    naming only one of them would work for one campaign and fail the other in a way that
    looks like a server fault.
    """
    found = {"127.0.0.1"}
    try:
        for info in socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET):
            found.add(info[4][0])
    except OSError:
        pass
    return sorted(found, key=lambda a: ipaddress.IPv4Address(a))


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path, default=REPO / "benchmark" / "certs")
    ap.add_argument("--openssl", default=default_openssl(),
                    help="the openssl executable; found automatically where one exists")
    ap.add_argument("--address", action="append", default=[],
                    help="an extra address to name in the certificate, repeatable")
    args = ap.parse_args(argv)

    args.out.mkdir(parents=True, exist_ok=True)
    cert = args.out / "bench.crt"
    key = args.out / "bench.key"

    addresses = sorted(set(local_addresses()) | set(args.address))
    # A name as well as the addresses. The generator sends no SNI for a literal address,
    # per RFC 6066, but a client that did send one has something to match.
    alt = ",".join(["DNS:localhost", *(f"IP:{a}" for a in addresses)])

    # openssl's documented one-liner for an EC key uses process substitution, which is a
    # shell feature, and this does not run under a shell. The curve parameters go to a
    # file first instead: two steps that are certain beat one that depends on how the
    # subprocess happened to be spawned.
    params = args.out / "ecparam.pem"
    try:
        subprocess.run([args.openssl, "ecparam", "-name", KEY_CURVE, "-out", str(params)],
                       check=True, capture_output=True, text=True)
        argv_openssl = [
            args.openssl, "req", "-x509", "-nodes",
            "-newkey", f"ec:{params}",
            "-keyout", str(key), "-out", str(cert),
            "-days", str(DAYS), "-subj", "/CN=coroute-benchmark",
            "-addext", f"subjectAltName={alt}",
        ]
        subprocess.run(argv_openssl, check=True, capture_output=True, text=True)
    except FileNotFoundError:
        print(f"no openssl executable named {args.openssl!r}. On a Windows host without "
              f"one, --openssl 'wsl openssl' works and writes to the same paths.",
              file=sys.stderr)
        return 2
    except subprocess.CalledProcessError as exc:
        print(f"openssl failed ({exc.returncode}):\n{exc.stderr.strip()}", file=sys.stderr)
        return 2
    finally:
        params.unlink(missing_ok=True)

    print(f"certificate {cert}")
    print(f"private key {key}")
    print(f"key         {KEY_ALGORITHM} {KEY_CURVE}, valid {DAYS} days")
    print(f"names       {alt}")
    print("\nRecord the key algorithm alongside any measurement taken with this: the "
          "server's private-key operation is part of every handshake it serves.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
