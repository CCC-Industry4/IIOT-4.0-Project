import cv2
from picamera2 import Picamera2
from flask import Flask, render_template, Response
import paho.mqtt
import paho.mqtt.publish as publish

picam2 = Picamera2()
picam2.start()

app = Flask(__name__)

def gen_frames():
    while True:
        image = picam2.capture_array()
        thing = cv2.rotate(image, cv2.ROTATE_90_COUNTERCLOCKWISE)
        thing = cv2.cvtColor(thing, cv2.COLOR_BGR2RGB)
        ret, buffer = cv2.imencode('.jpg', thing)
        frame = buffer.tobytes()

        yield(b'--frame\r\n'b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

@app.route('/stream')
def stream():
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == "__main__":
    publish.single("Smart Company/Neighborhood 2/Object Detection/Camera0", "192.168.10.2",hostname="192.168.10.2")
    app.run(host="192.168.10.2",port=81,debug=True, use_reloader=False)
