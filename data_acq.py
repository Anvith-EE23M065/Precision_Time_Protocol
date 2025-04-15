import socket
import csv
import threading

running = True

def listen_for_stop():
    global running
    input("Press Enter to Stop...\n")
    running = False

threading.Thread(target=listen_for_stop, daemon=True).start()

UDP_IP = "0.0.0.0"
UDP_PORT = 1234

master_file = open('master_data.csv', mode='w', newline='')
slave_file = open('slave_data.csv', mode='w', newline='')
master_writer = csv.writer(master_file)
slave_writer = csv.writer(slave_file)

master_writer.writerow(['Timestamp', 'Master Counter'])
slave_writer.writerow(['Timestamp', 'Slave Counter'])

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(0.5)  # 🔁 Set timeout to allow graceful stop

print("Logging started... Press Enter to stop.\n")

try:
    while running:
        try:
            data, addr = sock.recvfrom(1024)
            parts = data.decode('utf-8').split(',')
            if len(parts) != 3:
                continue
            device_type, timestamp, counter = parts
            if device_type == "MASTER":
                print(f"Master Data: {timestamp}, {counter}")
                master_writer.writerow([int(timestamp), int(counter)])
            elif device_type == "SLAVE":
                print(f"Slave Data: {timestamp}, {counter}")
                slave_writer.writerow([int(timestamp), int(counter)])
        except socket.timeout:
            continue  # Check for keyboard stop every 0.5 seconds
except Exception as e:
    print(f"Error: {e}")
finally:
    print("Stopping logging...")
    sock.close()
    master_file.close()
    slave_file.close()

