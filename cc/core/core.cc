#include "core.h"
#include "coreBindings.h"

#define FF_CV_PREDICATE(ff_name, ff_type, ff_ctor, ff_unwrapper)                      \
  struct ff_name##Predicate {                                                         \
    v8::Local<v8::Function> cb;                                                       \
    ff_name##Predicate(v8::Local<v8::Function> _cb) {                                 \
      cb = _cb;                                                                       \
    }                                                                                 \
    bool operator()(const ff_type& a, const ff_type& b) {                             \
      FF_VAL cbArgs[2];                                                               \
      cbArgs[0] = FF::newInstance(Nan::New(ff_ctor));                                           \
      cbArgs[1] = FF::newInstance(Nan::New(ff_ctor));                                           \
      ff_unwrapper(cbArgs[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked()) = a;                                        \
      ff_unwrapper(cbArgs[1]->ToObject(Nan::GetCurrentContext()).ToLocalChecked()) = b;                                        \
      Nan::AsyncResource resource("opencv4nodejs:Predicate::Constructor");            \
      return resource.runInAsyncScope(Nan::GetCurrentContext()->Global(),             \
        cb, 2, cbArgs).ToLocalChecked()->ToBoolean(Nan::GetCurrentContext()).ToLocalChecked()->Value();                              \
    }                                                                                 \
  }

FF_CV_PREDICATE(Point2, cv::Point2d, Point2::constructor, FF_UNWRAP_PT2_AND_GET);
FF_CV_PREDICATE(Point3, cv::Point3d, Point3::constructor, FF_UNWRAP_PT3_AND_GET);
FF_CV_PREDICATE(Vec2, cv::Vec2d, Vec2::constructor, FF_UNWRAP_VEC2_AND_GET);
FF_CV_PREDICATE(Vec3, cv::Vec3d, Vec3::constructor, FF_UNWRAP_VEC3_AND_GET);
FF_CV_PREDICATE(Vec4, cv::Vec4d, Vec4::constructor, FF_UNWRAP_VEC4_AND_GET);
FF_CV_PREDICATE(Mat, cv::Mat, Mat::constructor, FF_UNWRAP_MAT_AND_GET);

NAN_MODULE_INIT(Core::Init) {
  Mat::Init(target);
  Point::Init(target);
  Vec::Init(target);
  Size::Init(target);
  Rect::Init(target);
  RotatedRect::Init(target);
  TermCriteria::Init(target);

  Nan::SetMethod(target, "getBuildInformation", GetBuildInformation);
  Nan::SetMethod(target, "partition", Partition);
  Nan::SetMethod(target, "kmeans", Kmeans);
  Nan::SetMethod(target, "cartToPolar", CartToPolar);
  Nan::SetMethod(target, "cartToPolarAsync", CartToPolarAsync);
  Nan::SetMethod(target, "polarToCart", PolarToCart);
  Nan::SetMethod(target, "polarToCartAsync", PolarToCartAsync);
  Nan::SetMethod(target, "getNumThreads", GetNumThreads);
  Nan::SetMethod(target, "setNumThreads", SetNumThreads);
  Nan::SetMethod(target, "getThreadNum", GetThreadNum);
};

NAN_METHOD(Core::GetBuildInformation) {
  FF_METHOD_CONTEXT("Core::GetBuildInformation");

  v8::Local<v8::Value> ret = FF_NEW_STRING(cv::getBuildInformation());
  FF_RETURN(ret);
}

NAN_METHOD(Core::Partition) {
  FF_METHOD_CONTEXT("Core::Partition");
  FF_ARG_ARRAY(0, FF_ARR jsData);
  if (!info[1]->IsFunction()) {
    FF_THROW("expected arg 1 to be a function");
  }
  if (jsData->Length() < 2) {
    FF_THROW("expected data to contain atleast 2 elements");
  }

  v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(info[1]);
  FF_VAL data0 = jsData->Get(0);

  int numLabels = 0;
  std::vector<int> labels;
  if (FF_IS_INSTANCE(Point2::constructor, data0)) {
    std::vector<cv::Point2d> pts;
	Nan::TryCatch tryCatch;
	Point::unpackJSPoint2Array(pts, jsData);
	if (tryCatch.HasCaught()) {
		return info.GetReturnValue().Set(tryCatch.ReThrow());
	}
    numLabels = cv::partition(pts, labels, Point2Predicate(cb));
  }
  else if (FF_IS_INSTANCE(Point3::constructor, data0)) {
    std::vector<cv::Point3d> pts;
	Nan::TryCatch tryCatch;
	Point::unpackJSPoint3Array(pts, jsData);
	if (tryCatch.HasCaught()) {
		return info.GetReturnValue().Set(tryCatch.ReThrow());
	}
    numLabels = cv::partition(pts, labels, Point3Predicate(cb));
  }
  else if (FF_IS_INSTANCE(Vec2::constructor, data0)) {
    numLabels = cv::partition(Vec::unpackJSVec2Array(jsData), labels, Vec2Predicate(cb));
  }
  else if (FF_IS_INSTANCE(Vec3::constructor, data0)) {
    numLabels = cv::partition(Vec::unpackJSVec3Array(jsData), labels, Vec3Predicate(cb));
  }
  else if (FF_IS_INSTANCE(Vec4::constructor, data0)) {
    numLabels = cv::partition(Vec::unpackJSVec4Array(jsData), labels, Vec4Predicate(cb));
  }
  else if (FF_IS_INSTANCE(Mat::constructor, data0)) {
    std::vector<cv::Mat> mats;
    for (uint i = 0; i < jsData->Length(); i++) {
      mats.push_back(FF_UNWRAP_MAT_AND_GET(Nan::To<v8::Object>(jsData->Get(i)).ToLocalChecked()));
    }
    numLabels = cv::partition(mats, labels, MatPredicate(cb));
  }

  FF_OBJ ret = FF_NEW_OBJ();
  Nan::Set(ret, FF_NEW_STRING("labels"), IntArrayConverter::wrap(labels));
  Nan::Set(ret, FF_NEW_STRING("numLabels"), Nan::New(numLabels));
  FF_RETURN(ret);
}

NAN_METHOD(Core::Kmeans) {
  FF_METHOD_CONTEXT("Core::Kmeans");

  FF_ARG_ARRAY(0, FF_ARR jsData);
  
  if (jsData->Length() < 1) {
    FF_THROW("expected data to contain at least 1 element");
  }
  
  FF_VAL data0 = jsData->Get(0);
  


  FF_ARG_INT(1, int k);
  FF_ARG_INSTANCE(2, cv::TermCriteria termCriteria, TermCriteria::constructor, FF_UNWRAP_TERMCRITERA_AND_GET);
  FF_ARG_INT(3, int attempts);
  FF_ARG_INT(4, int flags);

  std::vector<int> labels;
  cv::Mat centersMat;
  
  if (FF_IS_INSTANCE(Point2::constructor, data0)) {
    std::vector<cv::Point2f> data;
	Nan::TryCatch tryCatch;
	Point::unpackJSPoint2Array(data, jsData);
	if (tryCatch.HasCaught()) {
		return info.GetReturnValue().Set(tryCatch.ReThrow());
	}
    cv::kmeans(data, k, labels, termCriteria, attempts, flags, centersMat);
  }
  else if (FF_IS_INSTANCE(Point3::constructor, data0)) {
    std::vector<cv::Point3f> data;
	Nan::TryCatch tryCatch;
	Point::unpackJSPoint3Array(data, jsData);
	if (tryCatch.HasCaught()) {
		return info.GetReturnValue().Set(tryCatch.ReThrow());
	}
    cv::kmeans(data, k, labels, termCriteria, attempts, flags, centersMat);
  } 
  else {
    FF_THROW("expected arg0 to be an Array of Points");
  }
  
  FF_OBJ ret = FF_NEW_OBJ();
  FF_PACK_ARRAY(jsLabels, labels);
  Nan::Set(ret, FF_NEW_STRING("labels"), jsLabels);

  if (FF_IS_INSTANCE(Point2::constructor, data0)) {
    std::vector<cv::Point2f> centers;
    for (int i = 0; i < centersMat.rows; i++) {
      centers.push_back(cv::Point2f(centersMat.at<float>(i, 0), centersMat.at<float>(i, 1)));
    }
    Nan::Set(ret, FF_NEW_STRING("centers"), Point::packJSPoint2Array<float>(centers));
  }
  else if (FF_IS_INSTANCE(Point3::constructor, data0)) {
    std::vector<cv::Point3f> centers;
    for (int i = 0; i < centersMat.rows; i++) {
      centers.push_back(cv::Point3f(centersMat.at<float>(i, 0), centersMat.at<float>(i, 1), centersMat.at<float>(i, 2)));
    }
    Nan::Set(ret, FF_NEW_STRING("centers"), Point::packJSPoint3Array<float>(centers));
  } 
  
  FF_RETURN(ret);
}

NAN_METHOD(Core::CartToPolar) {
  FF::SyncBinding(
    std::make_shared<CoreBindings::CartToPolarWorker>(),
    "Core::CartToPolar",
    info
  );
}

NAN_METHOD(Core::CartToPolarAsync) {
  FF::AsyncBinding(
    std::make_shared<CoreBindings::CartToPolarWorker>(),
    "Core::CartToPolarAsync",
    info
  );
}

NAN_METHOD(Core::PolarToCart) {
  FF::SyncBinding(
    std::make_shared<CoreBindings::PolarToCartWorker>(),
    "Core::PolarToCart",
    info
  );
}

NAN_METHOD(Core::PolarToCartAsync) {
  FF::AsyncBinding(
    std::make_shared<CoreBindings::PolarToCartWorker>(),
    "Core::PolarToCartAsync",
    info
  );
}

NAN_METHOD(Core::GetNumThreads) {
  FF_METHOD_CONTEXT("Core::GetNumThreads");
  FF_RETURN(IntConverter::wrap(cv::getNumThreads()));
}

NAN_METHOD(Core::SetNumThreads) {
  FF_METHOD_CONTEXT("Core::SetNumThreads");

  if(!FF_IS_INT(info[0])) {
    return Nan::ThrowError("Core::SetNumThreads expected arg0 to an int");
  }

  int32_t num = FF_CAST_INT(info[0]);

  cv::setNumThreads(num);
}

NAN_METHOD(Core::GetThreadNum) {
  FF_METHOD_CONTEXT("Core::GetNumThreads");
  FF_RETURN(IntConverter::wrap(cv::getThreadNum()));
}
