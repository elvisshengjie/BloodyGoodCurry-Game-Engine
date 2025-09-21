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
}
