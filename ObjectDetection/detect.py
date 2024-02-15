import cv2
import torch
#from ultralytics import YOLO
import time
import pandas
import paho.mqtt.publish as publish
from flask import Flask, render_template, Response

torch.hub.set_dir('./cache')
model = torch.hub.load('ultralytics/yolov5', 'yolov5s')
#model = YOLO("yolov8n.pt")

app = Flask(__name__)

width, height = 800, 600

def detect(vid): 
    _, pic = vid.read() 
    if not _:  
        raise Exception("vid is empty") 
    img = cv2.cvtColor(pic, cv2.COLOR_BGR2RGB) 
     
    result = model(img) 
     
    df = result.pandas().xyxy[0] 
    df.to_json('file.json') 
    thing = open('file.json') 
    output = thing.read() 
    print(output) 
    publish.single("IOT/test", output, hostname="192.168.10.2") 
    thing.close() 
     
    result.render() 
 
    out = cv2.cvtColor(img, cv2.COLOR_RGB2BGR) 
    return out 
 
def gen_frames(): 
    vid = cv2.VideoCapture("http://192.168.10.2:8081/") 
    vid.set(cv2.CAP_PROP_FRAME_WIDTH, width) 
    vid.set(cv2.CAP_PROP_FRAME_HEIGHT, height) 
 
    while True: 
        img = detect(vid);
        ret, buffer = cv2.imencode('.jpg', img) 
        frame = buffer.tobytes() 
        time.sleep(.1) 
     
        yield(b'--frame\r\n'b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n') 
     
@app.route('/video_feed') 
def video_feed(): 
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame') 
 
@app.route('/') 
def index(): 
    return render_template('index.html') 
 
if __name__ == "__main__": 
    app.run(host='192.168.10.2', debug=True)
