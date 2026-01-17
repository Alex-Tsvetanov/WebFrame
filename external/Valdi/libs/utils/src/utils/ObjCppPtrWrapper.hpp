/**
  Used to wrap another type. ObjC type encoding can be very large for complex
  C++ types, so when an ObjC method/field has a C++ type, wrap it in this to
  avoid the type encoding. If you do this, confirm it worked by checking the
  length of the type encoding before and after with: "objdump -macho
  -objc-meta-data YourObjectFile.o"
  **/
template<class T>
class ObjCppPtrWrapper {
public:
    T* o;
    ObjCppPtrWrapper(T* t) : o(t) {}
    ObjCppPtrWrapper() {
        o = nullptr;
    }
};
