import cv2
import torch
import time
import pandas
import json
import configparser
import threading
import paho.mqtt.publish as publish
from flask import Flask, render_template, Response

# config stuff
config = configparser.ConfigParser()
config.read('configfile.ini')

VIDEO_INPUT = config['YOLOv5']['VideoInput']
WIDTH = int(config['YOLOv5']['Width'])
HEIGHT = int(config['YOLOv5']['Height'])
CLASSES = json.loads(config['YOLOv5']['Classes'])
INFERENCE_SIZE = int(config['YOLOv5']['InferenceSize'])
IP = config['Flask']['IP']
PORT = config['Flask']['Port']
ADDRESS = IP + ":" + PORT

# load yolov5 model
torch.hub.set_dir('./cache')
model = torch.hub.load('ultralytics/yolov5', 'yolov5s', _verbose=False)
model.classes = CLASSES 

app = Flask(__name__)

class Camera:
    vid = cv2.VideoCapture(VIDEO_INPUT) 
    img = vid.read()[1]

    def __init__(self):
        self.width, self.height = WIDTH, HEIGHT
        self.vid.set(cv2.CAP_PROP_FRAME_WIDTH, self.width) 
        self.vid.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height) 

    def _detect(self):
        _, pic = self.vid.read() 
        
        if not _:  
            raise Exception("vid is empty") 
        
        readimg = cv2.cvtColor(pic, cv2.COLOR_BGR2RGB)  
        result = model(readimg, size=INFERENCE_SIZE) 
         
        df = result.pandas().xyxy[0] 
        thing = df.to_json()
        print(thing) 
        publish.single("IOT/test", thing, hostname="192.168.10.2") 
         
        result.render()
        
        self.img = cv2.cvtColor(readimg, cv2.COLOR_RGB2BGR) 
        return self.img

    def detect(self):
        while True:
            thread = threading.Thread(target=self._detect, args=())	
            thread.start()
            thread.join()

    def start(self):
        thread = threading.Thread(target=self.detect, args=()) 
        thread.start()
        return thread
 
def gen_frames(): 
    while True: 
        ret, buffer = cv2.imencode('.jpg', cam.img) 
        frame = buffer.tobytes() 
     
        yield(b'--frame\r\n'b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n') 
     
@app.route('/video_feed') 
def video_feed(): 
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame') 
 
@app.route('/') 
def index(): 
    return render_template('index.html') 
 
if __name__ == "__main__": 
    cam = Camera()
    cam.start()
    app.run(host=IP, debug=True,use_reloader=False)
