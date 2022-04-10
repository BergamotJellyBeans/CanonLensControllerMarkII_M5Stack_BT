// Queue.h
// By Steven de Salas
// Defines a templated (generic) class for a queue of things.
// Used for Arduino projects, just #include "Queue.h" and add this file via the IDE.

#ifndef STRINGQUEUE_H
#define STRINGQUEUE_H

#include <Arduino.h>

class StringQueue {
  private:
    int _front, _back, _count;
    String *_data;
    int _maxitems;
  public:
    StringQueue(int maxitems = 10) { 
      _front = 0;
      _back = 0;
      _count = 0;
      _maxitems = maxitems;
      _data = new String[maxitems + 1];   
    }
    ~StringQueue() {
      delete[] _data;  
    }
    inline int count();
    inline int front();
    inline int back();
    void push(const String &item);
    String peek();
    String pop();
    void clear();
};

inline int StringQueue::count() 
{
  return _count;
}

inline int StringQueue::front() 
{
  return _front;
}

inline int StringQueue::back() 
{
  return _back;
}

void StringQueue::push(const String &item)
{
  if(_count < _maxitems) { // Drops out when full
    _data[_back++]=item;
    ++_count;
    // Check wrap around
    if (_back > _maxitems)
      _back -= (_maxitems + 1);
  }
}

String StringQueue::pop() {
  if(_count <= 0) return String(); // Returns empty
  else {
    String result = _data[_front];
    _front++;
    --_count;
    // Check wrap around
    if (_front > _maxitems) 
      _front -= (_maxitems + 1);
    return result; 
  }
}

String StringQueue::peek() {
  if(_count <= 0) return String(); // Returns empty
  else return _data[_front];
}

void StringQueue::clear() 
{
  _front = _back;
  _count = 0;
}

#endif
