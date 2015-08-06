#pragma once

/**
* auto free the instance in the current scope, for instance, MyClass* ptr,
* which is a ptr and this class will:
*       1. free the ptr.
*       2. set ptr to NULL.
* Usage:
*       MyClass* po = new MyClass();
*       // ...... use po
*       SrsAutoFree(MyClass, po);
*/
#define SrsAutoFree(className, instance) \
    __SrsAutoFree<className> _auto_free_##instance(&instance)
template<class T>
class __SrsAutoFree
{
private:
    T** ptr;
public:
    /**
    * auto delete the ptr.
    */
    __SrsAutoFree(T** _ptr) {
        ptr = _ptr;
    }

    virtual ~__SrsAutoFree() {
        if (ptr == NULL || *ptr == NULL) {
            return;
        }

        delete *ptr;

        *ptr = NULL;
    }
};
