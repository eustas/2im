#include <node.h>

#include "../c/encoder.h"
#include "../c/image.h"

using namespace v8;

void wrapEncode(const FunctionCallbackInfo<v8::Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();
  auto makeString = [isolate](const char* str) {
    return String::NewFromUtf8(isolate, str, NewStringType::kNormal)
        .ToLocalChecked();
  };
  auto throwException = [isolate, &makeString](const char* text) {
    isolate->ThrowException(makeString(text));
  };
  auto getValue = [&makeString, &context] (Local<Object>& obj, const char* key) {
    return obj->Get(context, makeString(key));
  };

  if (args.Length() != 1) return throwException("Invalid arguments 1");

  Local<Value> arg = args[0];
  if (!arg->IsObject()) return throwException("Invalid arguments 2");
  Local<Object> imageData = arg->ToObject(context).ToLocalChecked();

  MaybeLocal<Value> maybeData = getValue(imageData, "data");
  if (maybeData.IsEmpty()) return throwException("Invalid arguments 3");
  Local<Value> dataVal = maybeData.ToLocalChecked();
  if (!dataVal->IsUint8ClampedArray()) throwException("Invalid arguments 4");
  Local<Uint8ClampedArray> data = dataVal.As<Uint8ClampedArray>();

  MaybeLocal<Value> maybeWidth = getValue(imageData, "width");
  if (maybeWidth.IsEmpty()) return throwException("Invalid arguments 5");
  Local<Value> widthVal = maybeWidth.ToLocalChecked();
  if (!widthVal->IsNumber()) return throwException("Invalid arguments 6");
  int32_t width = widthVal.As<Number>()->Int32Value(context).ToChecked();

  MaybeLocal<Value> maybeHeight = getValue(imageData, "height");
  if (maybeHeight.IsEmpty()) return throwException("Invalid arguments 7");
  Local<Value> heightVal = maybeHeight.ToLocalChecked();
  if (!heightVal->IsNumber()) return throwException("Invalid arguments 8");
  int32_t height = heightVal.As<Number>()->Int32Value(context).ToChecked();

  if (width <= 0 || height <= 0) return throwException("Invalid arguments 9");

  int32_t src_size = data->ByteLength();
  if (src_size != width * height * 4) return throwException("Invalid arguments 10");

  ArrayBuffer::Contents contents = data->Buffer()->GetContents();
  const uint8_t* src =
      static_cast<uint8_t*>(contents.Data()) + data->ByteOffset();

  twim::Image img = twim::Image::fromRgba(src, width, height);
  // TODO: check img.ok

  std::vector<uint8_t> encoded = twim::Encoder::encode(img, 200);

  Local<ArrayBuffer> out = ArrayBuffer::New(isolate, encoded.size());
  memcpy(out->GetContents().Data(), encoded.data(), encoded.size());

  args.GetReturnValue().Set(out);
}

void initialize(Local<Object> exports) {
  NODE_SET_METHOD(exports, "encode", wrapEncode);
}

NODE_MODULE(twim, initialize)
