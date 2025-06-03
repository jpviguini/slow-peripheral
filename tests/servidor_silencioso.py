import socket
import struct
import uuid

HEADER_FORMAT = "<16sIIIHBB"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 7033))

print("Servidor iniciado. Aguardando pacotes...")

reenvio_counter = {}

while True:
    data, addr = sock.recvfrom(2048)
    print(f"\n[Recebido de {addr}] {len(data)} bytes")

    if len(data) < HEADER_SIZE:
        print(f"[ERRO] Pacote muito pequeno! Esperado {HEADER_SIZE}, recebido {len(data)}")
        continue

    sid, sttl_flags, seqnum, acknum, window, fid, fo = struct.unpack(HEADER_FORMAT, data[:HEADER_SIZE])
    
    print(f"SID: {sid.hex()}")
    print(f"Flags: {sttl_flags:05b}")
    print(f"SEQNUM: {seqnum} | ACKNUM: {acknum}")
    print(f"WINDOW: {window} | FID: {fid} | FO: {fo}\n")

    if (sttl_flags & 0b10000) != 0:
        print("[INFO] Pacote de conexão recebido. Enviando resposta de setup...")

        new_sid = uuid.uuid4().bytes
        sttl = 0x00599000
        flags = 0b00010
        sttl_flags_resp = sttl | flags

        resp_seqnum = 0
        resp_acknum = seqnum
        resp_window = 4
        resp_fid = 0
        resp_fo = 0

        response = struct.pack(
            HEADER_FORMAT,
            new_sid,
            sttl_flags_resp,
            resp_seqnum,
            resp_acknum,
            resp_window,
            resp_fid,
            resp_fo
        )

        sock.sendto(response, addr)
        print("[INFO] Resposta enviada com novo SID.")
    else:
        if seqnum not in reenvio_counter:
            reenvio_counter[seqnum] = 1
        else:
            reenvio_counter[seqnum] += 1

        print(f"[IGNORADO] Reenvio do SEQNUM {seqnum} ({reenvio_counter[seqnum]}x)")
