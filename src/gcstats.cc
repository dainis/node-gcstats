#include <nan.h>

using namespace v8;

struct HeapInfo {
	size_t totalHeapSize;
	size_t totalHeapExecutableSize;
	size_t totalPhysicalSize;
	size_t usedHeapSize;
	size_t heapSizeLimit;
};

struct HeapData {
	HeapInfo* before;
	HeapInfo* after;
	uint64_t gcStartTime;
	uint64_t gcEndTime;
};

static Nan::Callback* afterGCCallback;

static HeapStatistics beforeGCStats;
uint64_t gcStartTime;
GCType gctype;

static NAN_GC_CALLBACK(recordBeforeGC) {
	//Docs say that new objects should not be created
	gcStartTime = uv_hrtime();
	Nan::GetHeapStatistics(&beforeGCStats);
}

static void copyHeapStats(HeapStatistics* stats, HeapInfo* info) {
	info->totalHeapSize = stats->total_heap_size();
	info->totalHeapExecutableSize = stats->total_heap_size_executable();
	#if NODE_MODULE_VERSION >= NODE_0_12_MODULE_VERSION
	info->totalPhysicalSize = stats->total_physical_size();
	#endif
	info->usedHeapSize = stats->used_heap_size();
	info->heapSizeLimit = stats->heap_size_limit();
}

static void formatStats(Local<Object> obj, HeapInfo* info) {
	Nan::Set(obj, Nan::New("totalHeapSize").ToLocalChecked(), Nan::New<Number>(info->totalHeapSize));
	Nan::Set(obj, Nan::New("totalHeapExecutableSize").ToLocalChecked(), Nan::New<Number>(info->totalHeapExecutableSize));
	Nan::Set(obj, Nan::New("usedHeapSize").ToLocalChecked(), Nan::New<Number>(info->usedHeapSize));
	Nan::Set(obj, Nan::New("heapSizeLimit").ToLocalChecked(), Nan::New<Number>(info->heapSizeLimit));
	#if NODE_MODULE_VERSION >= NODE_0_12_MODULE_VERSION
	Nan::Set(obj, Nan::New("totalPhysicalSize").ToLocalChecked(), Nan::New<Number>(info->totalPhysicalSize));
	#endif
}

static void formatStatDiff(Local<Object> obj, HeapInfo* before, HeapInfo* after) {
	Nan::Set(obj, Nan::New("totalHeapSize").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->totalHeapSize) - static_cast<double>(before->totalHeapSize)));
	Nan::Set(obj, Nan::New("totalHeapExecutableSize").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->totalHeapExecutableSize) - static_cast<double>(before->totalHeapExecutableSize)));
	Nan::Set(obj, Nan::New("usedHeapSize").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->usedHeapSize) - static_cast<double>(before->usedHeapSize)));
	Nan::Set(obj, Nan::New("heapSizeLimit").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->heapSizeLimit) - static_cast<double>(before->heapSizeLimit)));
	#if NODE_MODULE_VERSION >= NODE_0_12_MODULE_VERSION
	Nan::Set(obj, Nan::New("totalPhysicalSize").ToLocalChecked(), Nan::New<Number>(
		static_cast<double>(after->totalPhysicalSize) - static_cast<double>(before->totalPhysicalSize)));
	#endif
}

static void asyncAfter(uv_work_t* work, int status) {
	Nan::HandleScope scope;

	HeapData* data = static_cast<HeapData*>(work->data);

	Local<Object> obj = Nan::New<Object>();
	Local<Object> beforeGCStats = Nan::New<Object>();
	Local<Object> afterGCStats = Nan::New<Object>();

	formatStats(beforeGCStats, data->before);
	formatStats(afterGCStats, data->after);

	Local<Object> diffStats = Nan::New<Object>();
	formatStatDiff(diffStats, data->before, data->after);

	Nan::Set(obj, Nan::New("pause").ToLocalChecked(),
		Nan::New<Number>(static_cast<double>(data->gcEndTime - data->gcStartTime)));
	Nan::Set(obj, Nan::New("pauseMS").ToLocalChecked(),
		Nan::New<Number>(static_cast<double>((data->gcEndTime - data->gcStartTime) / 1000000)));
	Nan::Set(obj, Nan::New("gctype").ToLocalChecked(), Nan::New<Number>(gctype));
	Nan::Set(obj, Nan::New("before").ToLocalChecked(), beforeGCStats);
	Nan::Set(obj, Nan::New("after").ToLocalChecked(), afterGCStats);
	Nan::Set(obj, Nan::New("diff").ToLocalChecked(), diffStats);

	Local<Value> arguments[] = {obj};

	afterGCCallback->Call(1, arguments);

	delete data->before;
	delete data->after;
	delete data;
	delete work;
}

static void asyncWork(uv_work_t* work) {
	//can't create V8 objects here because this is different thread?
}

NAN_GC_CALLBACK(afterGC) {
	uv_work_t* work = new uv_work_t;

	HeapData* data = new HeapData;
	data->before = new HeapInfo;
	data->after = new HeapInfo;
	gctype = type;
	HeapStatistics stats;


	Nan::GetHeapStatistics(&stats);

	data->gcEndTime = uv_hrtime();
	data->gcStartTime = gcStartTime;

	copyHeapStats(&beforeGCStats, data->before);
	copyHeapStats(&stats, data->after);

	work->data = data;

	uv_queue_work(uv_default_loop(), work, asyncWork, asyncAfter);
}

static NAN_METHOD(AfterGC) {
	if(info.Length() != 1 || !info[0]->IsFunction()) {
		return Nan::ThrowError("Callback is required");
	}

	Local<Function> callbackHandle = info[0].As<Function>();
	afterGCCallback = new Nan::Callback(callbackHandle);

	Nan::AddGCEpilogueCallback(afterGC);
}

NAN_MODULE_INIT(init) {
	Nan::HandleScope scope;
	Nan::AddGCPrologueCallback(recordBeforeGC);

	Nan::Set(target,
		Nan::New("afterGC").ToLocalChecked(),
		Nan::GetFunction(
			Nan::New<FunctionTemplate>(AfterGC)).ToLocalChecked());
}

NODE_MODULE(gcstats, init)
