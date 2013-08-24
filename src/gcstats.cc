#include "node.h"
#include "v8.h"

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

static Persistent<Object> callbackContext;
static Persistent<Function> afterGCCallback;

static HeapStatistics beforeGCStats;
uint64_t gcStartTime;

static void recordBeforeGC(GCType, GCCallbackFlags) {
	//Docs say that new objects should not be created
	gcStartTime = uv_hrtime();
	V8::GetHeapStatistics(&beforeGCStats);
}

static void copyHeapStats(HeapStatistics* stats, HeapInfo* info) {
	info->totalHeapSize = stats->total_heap_size();
	info->totalHeapExecutableSize = stats->total_heap_size_executable();
	// info->totalPhysicalSize = stats->total_physical_size(); //total_physical_size is in headers but does not work wtf
	info->usedHeapSize = stats->used_heap_size();
	info->heapSizeLimit = stats->heap_size_limit();
}

static void formatStats(Handle<Object> obj, HeapInfo* info) {
	obj->Set(String::NewSymbol("totalHeapSize"), Number::New(info->totalHeapSize));
	obj->Set(String::NewSymbol("totalHeapExecutableSize"), Number::New(info->totalHeapExecutableSize));
	obj->Set(String::NewSymbol("usedHeapSize"), Number::New(info->usedHeapSize));
	obj->Set(String::NewSymbol("heapSizeLimit"), Number::New(info->heapSizeLimit));
}

static void formatStatDiff(Handle<Object> obj, HeapInfo* before, HeapInfo* after) {
	obj->Set(String::NewSymbol("totalHeapSize"), Number::New(
		static_cast<double>(after->totalHeapSize) - static_cast<double>(before->totalHeapSize)));
	obj->Set(String::NewSymbol("totalHeapExecutableSize"), Number::New(
		static_cast<double>(after->totalHeapExecutableSize) - static_cast<double>(before->totalHeapExecutableSize)));
	obj->Set(String::NewSymbol("usedHeapSize"), Number::New(
		static_cast<double>(after->usedHeapSize) - static_cast<double>(before->usedHeapSize)));
	obj->Set(String::NewSymbol("heapSizeLimit"), Number::New(
		static_cast<double>(after->heapSizeLimit) - static_cast<double>(before->heapSizeLimit)));
}

static void asyncAfter(uv_work_t* work, int status) {
	HeapData* data = static_cast<HeapData*>(work->data);

	Handle<Object> obj = Object::New();
	Handle<Object> beforeGCStats = Object::New();
	Handle<Object> afterGCStats = Object::New();

	formatStats(beforeGCStats, data->before);
	formatStats(afterGCStats, data->after);

	Handle<Object> diffStats = Object::New();
	formatStatDiff(diffStats, data->before, data->after);

	obj->Set(String::NewSymbol("pause"),
		Number::New(static_cast<double>(data->gcEndTime - data->gcStartTime)));
	obj->Set(String::NewSymbol("pauseMS"),
		Number::New(static_cast<double>((data->gcEndTime - data->gcStartTime) / 1000000)));
	obj->Set(String::NewSymbol("before"), beforeGCStats);
	obj->Set(String::NewSymbol("after"), afterGCStats);
	obj->Set(String::NewSymbol("diff"), diffStats);

	Handle<Value> arguments[] = {obj};

	afterGCCallback->Call(callbackContext, 1, arguments);

	delete data->before;
	delete data->after;
	delete data;
	delete work;
}

static void asyncWork(uv_work_t* work) {
	//can't create V8 objects here because this is different thread?
}

static void afterGC(GCType, GCCallbackFlags) {
	uv_work_t* work = new uv_work_t;

	HeapData* data = new HeapData;
	data->before = new HeapInfo;
	data->after = new HeapInfo;
	HeapStatistics stats;

	V8::GetHeapStatistics(&stats);

	data->gcEndTime = uv_hrtime();
	data->gcStartTime = gcStartTime;

	copyHeapStats(&beforeGCStats, data->before);
	copyHeapStats(&stats, data->after);

	work->data = data;

	uv_queue_work(uv_default_loop(), work, asyncWork, asyncAfter);
}

static Handle<Value> AfterGC(const Arguments& args) {

	if(args.Length() != 1 || !args[0]->IsFunction()) {
		ThrowException(Exception::TypeError(String::New("Callback is required")));
    	return Undefined();
	}

	Handle<Function> callback = Handle<Function>::Cast(args[0]);

	afterGCCallback = Persistent<Function>::New(callback);
	callbackContext  = Persistent<Object>::New(Context::GetCalling()->Global());

	V8::AddGCEpilogueCallback(afterGC);

	return Undefined();
}

void init(Handle<Object> exports) {
	V8::AddGCPrologueCallback(recordBeforeGC);
	exports->Set(String::NewSymbol("afterGC"), FunctionTemplate::New(AfterGC)->GetFunction());
}

NODE_MODULE(gcstats, init)