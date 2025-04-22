import socket
import csv
import threading
import os

running = True

def listen_for_stop():
    global running
    input("Press Enter to Stop...\n")
    running = False

threading.Thread(target=listen_for_stop, daemon=True).start()

UDP_IP = "0.0.0.0"
UDP_PORT = 1234

# Get directory of current script
script_dir = os.path.dirname(os.path.abspath(__file__))
print("Script directory:", script_dir)

# Create full paths for CSV files
master_path = os.path.join(script_dir, 'master_data.csv')
slave_path = os.path.join(script_dir, 'slave_data.csv')

# Open files
try:
    master_file = open(master_path, mode='w', newline='')
    slave_file = open(slave_path, mode='w', newline='')
    print("CSV files created successfully in script directory.")
except Exception as e:
    print(f"Error opening files: {e}")
    exit(1)

master_writer = csv.writer(master_file)
slave_writer = csv.writer(slave_file)

master_writer.writerow(['Timestamp', 'Master Counter'])
slave_writer.writerow(['Timestamp', 'Slave Counter'])

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
try:
    sock.bind((UDP_IP, UDP_PORT))
    print(f"Socket bound to {UDP_IP}:{UDP_PORT}")
except Exception as e:
    print(f"Socket error: {e}")
    master_file.close()
    slave_file.close()
    exit(1)

sock.settimeout(0.5)

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
            continue
        except Exception as e:
            print(f"Data processing error: {e}")
except KeyboardInterrupt:
    print("Stopped by keyboard interrupt.")
finally:
    print("Stopping logging...")
    sock.close()
    master_file.close()
    slave_file.close()
    print("Files closed.")
