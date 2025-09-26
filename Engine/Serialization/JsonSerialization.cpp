#include "JsonSerialization.h"
#include <stdexcept>

// example of a json file
//{
//    "GameObject": {
//        "Transform": {
//            "x": 100,
//                "y" : 200
//        },
//            "Render" : {
//            "sprite": "player.png",
//                "layer" : 1
//        },
//            "Physics" : {
//            "mass": 1.0,
//                "gravity" : true
//        }
//    }
//}

namespace Framework
{
    bool JsonSerializer::Open(const std::string& file)
    {
        std::ifstream stream(file);
        if (!stream.is_open()) return false;

        stream >> root;  //parse the entire file into JSON object
        objectStack = {};   //clear stack
        objectStack.push(&root);   //push the object of root onto the stack 
                                   //Now the current object reading is from the top of the stack
        return true;
    }

    bool JsonSerializer::IsGood()
    {
        return !objectStack.empty(); //check if at least one object
    }

    bool JsonSerializer::EnterObject(const std::string& key)
    {
        json* current = objectStack.top();  
        if (current->contains(key) && (*current)[key].is_object())  //check if key exsits and make sure there is an object
        {
            objectStack.push(&(*current)[key]); // now current object become the nested one
                                                // example {
                                                //
                                                //"Player":{ "Health": 100}
                                                // after push now stack top point to { "Health": 100}
            return true;
        }
        return false;
    }

    void JsonSerializer::ExitObject()
    {
        if (objectStack.size() > 1)              
            objectStack.pop();      // leave current object
    }

    bool JsonSerializer::HasKey(const std::string& key) const
    {
        //check whether the current object contains the given key
        json* current = objectStack.top();
        return current->contains(key);
    }

    void JsonSerializer::ReadInt(const std::string& key, int& out)
    {
        //Read the value at key as an int and store out
        json* current = objectStack.top();
        out = (*current)[key].get<int>();
    }

    void JsonSerializer::ReadFloat(const std::string& key, float& out)
    {
        //Read the value at key as an float and store out
        json* current = objectStack.top();
        out = (*current)[key].get<float>();
    }

    void JsonSerializer::ReadString(const std::string& key, std::string& out)
    {
        //Read the value at string as an float and store out
        json* current = objectStack.top();
        out = (*current)[key].get<std::string>();
    }
    bool JsonSerializer::EnterArray(const std::string& key)
    {
        json* cur = objectStack.top(); // get the current Json object on top of the stack
        //Check if the current object had field named 'key'
        //AND that field is actually an array
        if (cur->contains(key) && (*cur)[key].is_array()) { objectStack.push(&(*cur)[key]); //Push pointer to this Array on the stack
            //This array become the current scope
          return true; 
        }
            return false;// key not found or not an array nothing pushed
    }
    void JsonSerializer::ExitArray() 
    {
        if (objectStack.size() > 1)
            objectStack.pop();
    }
    size_t JsonSerializer::ArraySize() const
    {
        json* cur = objectStack.top(); 
        return cur->is_array() ? cur->size() : 0;
    }
    bool JsonSerializer::EnterIndex(size_t i)
    {
        json* cur = objectStack.top();
        if (cur->is_array() && i < cur->size()) 
        { //push i as the new "current"
            objectStack.push(&(*cur)[i]); 
            //now inside Gameobject[i]
            return true; 
        }
        return false;
    }
}
