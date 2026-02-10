import time
import sys
import threading
from flask import Flask, render_template, request
from flask_socketio import SocketIO
from SX127x.LoRa import *
from SX127x.board_config import BOARD

# --- CONFIGURATION ---
app = Flask(__name__)
app.config['SECRET_KEY'] = 'emergency_satellite_secret'
socketio = SocketIO(app, cors_allowed_origins='*', async_mode='threading') # Changed to threading mode for better Pi stability

# Global stats
stats = {
    "total_packets": 0,
    "last_rssi": 0,
    "last_seen": "Waiting...",
    "nodes_active": set()
}

# --- LORA CLASS ---
BOARD.setup()

class SatelliteLoRa(LoRa):
    def __init__(self, verbose=False):
        super(SatelliteLoRa, self).__init__(verbose)
        self.set_mode(MODE.SLEEP)
        self.set_dio_mapping([0] * 6)

        # --- EXACT MATCH SETTINGS ---
        self.set_freq(433.0)
        self.set_sync_word(0xA5)
        self.set_spreading_factor(7)
        self.set_bw(BW.BW125)
        self.set_coding_rate(CODING_RATE.CR4_5)
        self.set_pa_config(pa_select=1)

    def on_rx_done(self):
        self.clear_irq_flags(RxDone=1)
        payload = self.read_payload(nocheck=True)

        try:
            # 1. Decode Data
            msg = bytes(payload).decode("utf-8", 'ignore')

            # 2. Get Signal Stats (FIXED METHOD HERE)
            rssi = self.get_rssi_value()
            try:
                snr = self.get_pkt_snr_value()
            except:
                snr = 0 # Fallback if library version differs

            # 3. Update Stats
            stats["total_packets"] += 1
            stats["last_rssi"] = rssi
            stats["last_seen"] = time.strftime("%H:%M:%S")

            # 4. Push to Web Dashboard
            socketio.emit('packet_data', {
                'msg': msg,
                'rssi': rssi,
                'snr': snr,
                'timestamp': stats["last_seen"],
                'count': stats["total_packets"]
            })

            print(f"[RX] {msg} | RSSI: {rssi} | SNR: {snr}")

        except Exception as e:
            print(f"Decode Error: {e}")

        # Reset mode
        self.set_mode(MODE.SLEEP)
        self.reset_ptr_rx()
        self.set_mode(MODE.RXCONT)

# --- BACKGROUND THREAD ---
def lora_thread_task():
    print(">>> LoRa Satellite Active on 433MHz...")
    lora = SatelliteLoRa(verbose=False)
    lora.set_mode(MODE.RXCONT)

    while True:
        time.sleep(1)

# --- FLASK ROUTES ---
@app.route('/')
def index():
    return render_template('dashboard.html')

@app.route('/api/status')
def get_status():
    # Convert set to list for JSON serialization if needed
    return stats

# --- MAIN ---
if __name__ == '__main__':
    t = threading.Thread(target=lora_thread_task)
    t.daemon = True
    t.start()

    print(">>> Web Interface: http://0.0.0.0:5000")
    # Allow external connections
    socketio.run(app, host='0.0.0.0', port=5000, debug=False)