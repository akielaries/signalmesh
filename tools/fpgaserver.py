#!/usr/bin/env python3
"""
FPGA programming "Server"
"""

import socket
import subprocess
import tempfile
import os
import json
import threading
import logging
from pathlib import Path


# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)


class FPGAServer:
    def __init__(self, host='0.0.0.0', port=65432):
        self.host = host
        self.port = port
        self.running = False
        
    def handle_client(self, conn, addr):
        """Handle a single client connection"""
        try:
            logger.info(f"Connection from {addr}")
            
            # Receive header (first 4 bytes = file size)
            header = conn.recv(4)
            if len(header) < 4:
                logger.error("Invalid header received")
                return
                
            file_size = int.from_bytes(header, 'big')
            logger.info(f"Receiving {file_size} bytes")
            
            # Receive board type length (1 byte)
            board_len = int.from_bytes(conn.recv(1), 'big')
            board_type = conn.recv(board_len).decode('utf-8')
            logger.info(f"Board type: {board_type}")
            
            # Receive bitstream data
            received = 0
            chunks = []
            while received < file_size:
                chunk = conn.recv(min(4096, file_size - received))
                if not chunk:
                    break
                chunks.append(chunk)
                received += len(chunk)
            
            if received != file_size:
                logger.error(f"Incomplete transfer: {received}/{file_size} bytes")
                conn.sendall(b"ERROR: Incomplete file transfer")
                return
            
            # Save to temporary file
            bitstream_data = b''.join(chunks)
            with tempfile.NamedTemporaryFile(mode='wb', suffix='.fs', delete=False) as f:
                temp_path = f.name
                f.write(bitstream_data)
                logger.info(f"Saved to temporary file: {temp_path}")
            
            try:
                # Program the FPGA
                cmd = ['openFPGALoader', '-v', '-b', board_type, temp_path]
                logger.info(f"Executing: {' '.join(cmd)}")
                
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    timeout=30  # 30 second timeout
                )
                
                # Send result back to client
                response = {
                    'returncode': result.returncode,
                    'stdout': result.stdout,
                    'stderr': result.stderr
                }
                conn.sendall(json.dumps(response).encode('utf-8'))
                
                logger.info(f"Programming completed with return code: {result.returncode}")
                
            except subprocess.TimeoutExpired:
                logger.error("Programming timeout")
                conn.sendall(json.dumps({
                    'returncode': -1,
                    'stdout': '',
                    'stderr': 'Timeout expired'
                }).encode('utf-8'))
                
            except Exception as e:
                logger.error(f"Programming error: {e}")
                conn.sendall(json.dumps({
                    'returncode': -1,
                    'stdout': '',
                    'stderr': str(e)
                }).encode('utf-8'))
                
            finally:
                # Clean up temporary file
                if os.path.exists(temp_path):
                    os.unlink(temp_path)
                    
        except Exception as e:
            logger.error(f"Client handling error: {e}")
        finally:
            conn.close()
    
    def start(self):
        """Start the server"""
        self.running = True
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind((self.host, self.port))
            server_socket.listen(5)
            
            logger.info(f"FPGA Server listening on {self.host}:{self.port}")
            
            while self.running:
                try:
                    conn, addr = server_socket.accept()
                    client_thread = threading.Thread(
                        target=self.handle_client,
                        args=(conn, addr)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                except KeyboardInterrupt:
                    logger.info("Shutting down server...")
                    break
                except Exception as e:
                    logger.error(f"Server error: {e}")
    
    def stop(self):
        """Stop the server"""
        self.running = False


def main():
    print("=" * 60)
    print("=" * 60)
    print("\nStarting FPGA Server...")
    
    server = FPGAServer()
    server.start()

if __name__ == '__main__':
    main()
