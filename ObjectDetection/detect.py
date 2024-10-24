import cv2
import torch
import time
import pandas
import json
import configparser
import threading
import paho.mqtt 
import paho.mqtt.publish as publish
import paho.mqtt.subscribe as subscribe
from flask import Flask, render_template, Response

# config stuff
config = configparser.ConfigParser()
config.read('configfile.ini')

#VIDEO_INPUT = int(config['YOLOv5']['VideoInput'])
VIDEO_INPUT = '/dev/video0'
WIDTH = int(config['YOLOv5']['Width'])
HEIGHT = int(config['YOLOv5']['Height'])
CLASSES = json.loads(config['YOLOv5']['Classes'])
INFERENCE_SIZE = int(config['YOLOv5']['InferenceSize'])
IP = config['Flask']['IP']
PORT = config['Flask']['Port']
ADDRESS = IP + ":" + PORT

# load yolov5 model
torch.hub.set_dir('./cache')
model = torch.hub.load('ultralytics/yolov5', 'yolov5n', '--img 160 --half', _verbose=False)
#model = torch.hub.load('ultralytics/yolov5', 'custom', 'best.pt', force_reload=False)
model.classes = CLASSES 


# initialize flask
app = Flask(__name__)

class Camera:
    # do camera stuff
    url = VIDEO_INPUT
    vid = cv2.VideoCapture(-1)
    img = vid.read()[0]

    pic = img
    prev = "{}"
    changed = False

    def __init__(self):
        self.width, self.height = WIDTH, HEIGHT
        self.vid.set(cv2.CAP_PROP_FRAME_WIDTH, self.width) 
        self.vid.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height) 

    def _detect(self):
        # read img
        try:
            readimg = cv2.cvtColor(self.pic, cv2.COLOR_BGR2RGB) 
        except:
            return {}

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
                namespace = "Smart Company/Neighborhood 2/Object Detection/" + obj + "/" + data
                publish.single(namespace, None, hostname="192.168.10.2") 

        # publish new data
        for obj in jsonData:
            count += 1
            for data in jsonData[obj]:
                namespace = "Smart Company/Neighborhood 2/Object Detection/" + obj + "/" + data
                publish.single(namespace, jsonData[obj][data], hostname="192.168.10.2") 

        publish.single("Smart Company/Neighborhood 2/Object Detection/count", count, hostname="192.168.10.2")
        publish.single("Smart Company/Neighborhood 2/Object Detection/raw", inputData, hostname="192.168.10.2")

    def detect(self):
        while True:
            start = time.process_time();
            self._detect()

    def _reader(self):
        while True:
            if self.vid.isOpened():
                while True:
                    if self.changed:
                        print(self.changed)
                        self.vid = cv2.VideoCapture(self.url) 
                        self.changed = False

                    _, self.pic = self.vid.read() 
                    if not _:
                        self.vid.release()
                        break
            else:
                while True:
                    if self.changed:
                        print(VIDEO_INPUT)
                        self.vid = cv2.VideoCapture(self.url) 
                        self.changed = False

                    self.vid = cv2.VideoCapture(self.url) 
                    if self.vid.isOpened():
                        break
                    else:
                        self.vid.release();



    def start(self):
        thread = threading.Thread(target=self.detect, args=()) 
        thread1 = threading.Thread(target=self._reader, args=()) 
        thread.start()
        thread1.start()
        return thread

cam = Camera()
def on_message(client, userdata, msg):
    VIDEO_INPUT = 'http://' + msg.payload.decode("utf-8") + ':81/stream'
    print(VIDEO_INPUT)
    cam.changed = True
    cam.url = VIDEO_INPUT

client = paho.mqtt.client.Client(paho.mqtt.client.CallbackAPIVersion.VERSION2, "client")
client.on_message = on_message;
client.connect("192.168.10.2", 1883)
client.subscribe("Smart Company/Neighborhood 2/Object Detection/ip")
client.loop_start()

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
    cam.start()
    app.run(host=IP, debug=True,use_reloader=False)
