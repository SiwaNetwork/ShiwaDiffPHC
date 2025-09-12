#!/usr/bin/env python3
"""
Простой тестовый веб-сервер для демонстрации веб-интерфейса ShiwaDiffPHC
"""

import http.server
import socketserver
import json
import os
from datetime import datetime

class ShiwaDiffPHCHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=os.getcwd(), **kwargs)
    
    def do_GET(self):
        if self.path == '/':
            # Serve main web interface
            self.path = '/web_interface.html'
            return super().do_GET()
        elif self.path == '/api/status':
            self.send_api_response({
                "measuring": False,
                "deviceCount": 2,
                "measurementCount": 15,
                "lastUpdate": datetime.now().isoformat(),
                "avgDifference": 12.5
            })
        elif self.path == '/api/history':
            # Generate sample history data
            history = []
            for i in range(10):
                history.append({
                    "timestamp": datetime.now().isoformat(),
                    "difference": 10.0 + i * 0.5,
                    "success": True,
                    "deviceCount": 2
                })
            self.send_api_response({"history": history})
        elif self.path == '/api/devices':
            self.send_api_response({"devices": [0, 1]})
        else:
            super().do_GET()
    
    def do_POST(self):
        if self.path == '/api/start':
            self.send_api_response({"status": "started"})
        elif self.path == '/api/stop':
            self.send_api_response({"status": "stopped"})
        elif self.path == '/api/refresh':
            self.send_api_response({"status": "refreshed"})
        else:
            self.send_error(404, "Not Found")
    
    def send_api_response(self, data):
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
        
        response = json.dumps(data, ensure_ascii=False, indent=2)
        self.wfile.write(response.encode('utf-8'))
    
    def end_headers(self):
        self.send_header('Cache-Control', 'no-cache')
        super().end_headers()

def main():
    PORT = 8081
    
    print(f"Запуск тестового веб-сервера ShiwaDiffPHC на порту {PORT}")
    print(f"Откройте в браузере: http://localhost:{PORT}")
    print("Нажмите Ctrl+C для остановки")
    
    with socketserver.TCPServer(("", PORT), ShiwaDiffPHCHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nСервер остановлен")

if __name__ == "__main__":
    main()
