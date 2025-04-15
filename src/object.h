#ifndef CWS_OBJECT_H
#define CWS_OBJECT_H
typedef struct Object Object;

typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjectType;

struct Object
{
    ObjectType type;
};

typedef struct ObjectString ObjectString;

struct ObjectString
{
    Object *object;
    char *string;
    int length;
};

#define OBJ_TYPE(obj)(obj->object->type)
#endif // !CWS_OBJECT_H
