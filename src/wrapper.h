/*
 * A simple wrapper library for Khoi Hoang's ESP32 AsyncMQTT library
 */

#pragma once

#ifndef ASYNC_MQTT_ESP32_WRAPPER
#define ASYNC_MQTT_ESP32_WRAPPER

#define SUB_FLAG 1
#define PUB_FLAG 2

#include <AsyncMQTT_ESP32.h>
#include <unordered_map>
#include <string>

// Topic Class
class Topic{
    // Topic name and value
    std::string topicName, value;
    // Flags if the topic is changed
    char changed;
    
    public:
      // CStors
      Topic(){}

      Topic(const char* topic){
        topicName = std::string(topic);
      }

      // Destructor
      ~Topic(){
      }

      const char* getName() const {
        return topicName.c_str();
      }

      const char* getVal() const{
         return value.c_str();
      }

      bool operator==(const char* cmp){
        if (value.c_str() == nullptr) return false;
        return (*cmp == *value.c_str());
      }

      bool operator!=(const char* cmp){
        if (value.c_str() == nullptr) return true;
        return (*cmp != *value.c_str());
      }

      Topic& operator=(char* newVal){
        changed = changed | PUB_FLAG; // set the PUB_FLAG bit to true
        value = std::string(newVal);
        return *this;
      }

      Topic& operator=(int newVal){
        changed = changed | PUB_FLAG; // set the PUB_FLAG bit to true
        this->setVal((char*)std::to_string(newVal).c_str()); // very jank, fix later
        return *this;
      }

      bool published(){
        return changed & PUB_FLAG;
      }

      bool subscribed(){
        return changed & SUB_FLAG;
      }

      // Function that sets the value of the topic
      void setVal(char* val) {
        value = std::string(val);
      }
      
      void setPublished(bool val){
        if (!val) changed = changed & !PUB_FLAG;
        else changed = changed | PUB_FLAG;
      }
      void setSubscribe(bool val){
        if (!val) changed = changed & !SUB_FLAG;
        else changed = changed | SUB_FLAG;
      }
};

class MqttClient{
  static inline std::unordered_map<std::string, Topic*> cache;
  static inline std::string mainTopic;


  static void onMqttMessage(char *topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                     const size_t& len, const size_t& index, const size_t& total)
  {
    // store message in cache
    std::string topicName(topic);
    if (cache.count(topicName)){
      if (!payload)
        cache[topicName]->setVal("");
      else
    	  cache[topicName]->setVal(payload);
      cache[topicName]->setSubscribe(true);
    }
  }

  static void onMqttConnect(bool sessionPresent){
    client.subscribe((mainTopic + "/#").c_str(), 2);
    client.publish(mainTopic.c_str(), 0, true, "ESP32 Test");
  }
public:
  static inline AsyncMqttClient client;

  MqttClient(){}
  MqttClient(const char* MQTTClientIP, int Port, const char* newTopic){
    client.setServer(MQTTClientIP, Port);
    mainTopic = std::string(newTopic);

    client.onMessage(onMqttMessage);
    client.onConnect(onMqttConnect);
  }
  ~MqttClient(){};

  void addTopic(Topic* topic){
    cache.emplace(mainTopic + "/" + std::string(topic->getName()), topic);
  } 
  void connect(){
    client.connect();
  }
  void publish() const {
    for (std::pair<std::string, Topic*> topic: cache){
      if (topic.second->published()){
        topic.second->setPublished(false);
        client.publish(topic.first.c_str(), 0, true, topic.second->getVal());
      }
      if (topic.second->subscribed()){
        topic.second->setSubscribe(false);
      }
    } 
  }
  const char* getValue(const char* topic) const {
    std::string topicName(topic);
    if (cache.count(topicName)) return cache.at(topicName)->getName(); 
    return NULL;
  }
};
#endif
