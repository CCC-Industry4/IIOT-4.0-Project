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

VIDEO_INPUT = int(config['YOLOv5']['VideoInput'])
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

# initialize flask
app = Flask(__name__)

class Camera:
    # do camera stuff
    vid = cv2.VideoCapture(VIDEO_INPUT) 
    img = vid.read()[1]
    pic = img
    prev = "{}"

    def __init__(self):
        self.width, self.height = WIDTH, HEIGHT
        self.vid.set(cv2.CAP_PROP_FRAME_WIDTH, self.width) 
        self.vid.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height) 

    def _detect(self):
        # if there is no camera, throw excpetion
    
        # read img
        readimg = cv2.cvtColor(self.pic, cv2.COLOR_BGR2RGB)  
    
        # do object detection
        result = model(readimg, size=INFERENCE_SIZE) 
    
        # parse string
        pd_df = result.pandas()
        df = pd_df.xyxy[0].transpose()
    
        # publish
        self.publish(df.to_json())
    
        # render img
        result.render()
    
        # display img
        self.img = cv2.cvtColor(readimg, cv2.COLOR_RGB2BGR) 
        return self.img

    def publish(self, inputData):
        # convert json string to dict
        jsonData = json.loads(inputData);

        # load previous data
        previousData = json.loads(self.prev)
        self.prev = inputData
        count = 0
        # cleanup previous data
        for obj in previousData:
            for data in previousData[obj]:
                namespace = "IOT/ObjectDetection/" + obj + "/" + data
                publish.single(namespace, None, hostname="192.168.10.2") 

        # publish new data
        for obj in jsonData:
            count += 1
            for data in jsonData[obj]:
                namespace = "IOT/ObjectDetection/" + obj + "/" + data
                publish.single(namespace, jsonData[obj][data], hostname="192.168.10.2") 

        publish.single("IOT/ObjectDetection/count", count, hostname="192.168.10.2")
        publish.single("IOT/ObjectDetection/raw", inputData, hostname="192.168.10.2")

    def detect(self):
        while True:
            vid = cv2.VideoCapture("http://192.168.10.161:8081") 
            start = time.process_time();
            self._detect()
            print(time.process_time() - start);

    def _reader(self):
         while True:
             _, self.pic = self.vid.read() 


    def start(self):
        thread = threading.Thread(target=self.detect, args=()) 
        thread1 = threading.Thread(target=self._reader, args=()) 
        thread.start()
        thread1.start()
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
