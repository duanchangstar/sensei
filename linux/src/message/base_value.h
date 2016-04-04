/**
 * @brief Value messages definition
 * @copyright MIND Music Labs AB, Stockholm
 *
 * Base Value class and macros for quick subclasses definition.
 * This is intended for internal module use, if you need to define special command sub-classes do it so in
 * message/value_defs.h
 *
 */

#ifndef SENSEI_BASE_VALUE_H
#define SENSEI_BASE_VALUE_H

#include "message/base_message.h"

namespace sensei {

class MessageFactory;

/**
 * @brief Abstract base class for values.
 */
class Value : public BaseMessage
{
public:
    virtual ~Value()
    {
    }

    SENSEI_MESSAGE_DECLARE_NON_COPYABLE(Value)

    bool    is_value() override
    {
        return true;
    }

protected:
    Value(const int sensor_index, const uint32_t timestamp=0) :
            BaseMessage(sensor_index, timestamp)
    {
    }

};

#define SENSEI_DECLARE_VALUE(ClassName, InternalType, representation_prefix) \
class ClassName : public Value \
{ \
public: \
    SENSEI_MESSAGE_CONCRETE_CLASS_PREAMBLE(ClassName) \
    std::string representation() override \
    {\
        return std::string(representation_prefix);\
    }\
    InternalType value()\
    {\
        return _value;\
    }\
private:\
    ClassName(const int sensor_index,\
              const InternalType value,\
              const uint32_t timestamp=0) :\
        Value(sensor_index, timestamp),\
        _value(value)\
    {\
    }\
    InternalType _value;\
}


}; // namespace sensei

#endif // SENSEI_BASE_VALUE_H
