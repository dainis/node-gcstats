#include "node.h"
#include "v8.h"
#include "nan.h"
#include <stdio.h>

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

static NanCallback* afterGCCallback;

static HeapStatistics beforeGCStats;
uint64_t gcStartTime;

#if NODE_MODULE_VERSION >=14
static void recordBeforeGC(Isolate*, GCType, GCCallbackFlags) {
#else
static void recordBeforeGC(GCType, GCCallbackFlags) {
#endif
	//Docs say that new objects should not be created
	gcStartTime = uv_hrtime();
	NanGetHeapStatistics(&beforeGCStats);
}

static void copyHeapStats(HeapStatistics* stats, HeapInfo* info) {
	info->totalHeapSize = stats->total_heap_size();
	info->totalHeapExecutableSize = stats->total_heap_size_executable();
	// info->totalPhysicalSize = stats->total_physical_size(); //total_physical_size is in headers but does not work wtf
	info->usedHeapSize = stats->used_heap_size();
	info->heapSizeLimit = stats->heap_size_limit();
}

static void formatStats(Handle<Object> obj, HeapInfo* info) {
	obj->Set(NanSymbol("totalHeapSize"), NanNew<Number>(info->totalHeapSize));
	obj->Set(NanSymbol("totalHeapExecutableSize"), NanNew<Number>(info->totalHeapExecutableSize));
	obj->Set(NanSymbol("usedHeapSize"), NanNew<Number>(info->usedHeapSize));
	obj->Set(NanSymbol("heapSizeLimit"), NanNew<Number>(info->heapSizeLimit));
}

static void formatStatDiff(Handle<Object> obj, HeapInfo* before, HeapInfo* after) {
	obj->Set(NanSymbol("totalHeapSize"), NanNew<Number>(
		static_cast<double>(after->totalHeapSize) - static_cast<double>(before->totalHeapSize)));
	obj->Set(NanSymbol("totalHeapExecutableSize"), NanNew<Number>(
		static_cast<double>(after->totalHeapExecutableSize) - static_cast<double>(before->totalHeapExecutableSize)));
	obj->Set(NanSymbol("usedHeapSize"), NanNew<Number>(
		static_cast<double>(after->usedHeapSize) - static_cast<double>(before->usedHeapSize)));
	obj->Set(NanSymbol("heapSizeLimit"), NanNew<Number>(
		static_cast<double>(after->heapSizeLimit) - static_cast<double>(before->heapSizeLimit)));
}

static void asyncAfter(uv_work_t* work, int status) {
	NanScope();

	HeapData* data = static_cast<HeapData*>(work->data);

	Handle<Object> obj = NanNew<Object>();
	Handle<Object> beforeGCStats = NanNew<Object>();
	Handle<Object> afterGCStats = NanNew<Object>();

	formatStats(beforeGCStats, data->before);
	formatStats(afterGCStats, data->after);

	Handle<Object> diffStats = NanNew<Object>();
	formatStatDiff(diffStats, data->before, data->after);

	obj->Set(NanSymbol("pause"),
		NanNew<Number>(static_cast<double>(data->gcEndTime - data->gcStartTime)));
	obj->Set(NanSymbol("pauseMS"),
		NanNew<Number>(static_cast<double>((data->gcEndTime - data->gcStartTime) / 1000000)));
	obj->Set(NanSymbol("before"), beforeGCStats);
	obj->Set(NanSymbol("after"), afterGCStats);
	obj->Set(NanSymbol("diff"), diffStats);

	Handle<Value> arguments[] = {obj};

	afterGCCallback->Call(1, arguments);

	delete data->before;
	delete data->after;
	delete data;
	delete work;
}

static void asyncWork(uv_work_t* work) {
	//can't create V8 objects here because this is different thread?
}

#if NODE_MODULE_VERSION >=14
static void afterGC(Isolate*, GCType, GCCallbackFlags) {
#else
static void afterGC(GCType, GCCallbackFlags) {
#endif
	uv_work_t* work = new uv_work_t;

	HeapData* data = new HeapData;
	data->before = new HeapInfo;
	data->after = new HeapInfo;
	HeapStatistics stats;

	NanGetHeapStatistics(&stats);

	data->gcEndTime = uv_hrtime();
	data->gcStartTime = gcStartTime;

	copyHeapStats(&beforeGCStats, data->before);
	copyHeapStats(&stats, data->after);

	work->data = data;

	uv_queue_work(uv_default_loop(), work, asyncWork, asyncAfter);
}

static NAN_METHOD(AfterGC) {
	NanScope();

	if(args.Length() != 1 || !args[0]->IsFunction()) {
		return NanThrowError("Callback is required");
	}

	Local<Function> callbackHandle = args[0].As<Function>();
	afterGCCallback = new NanCallback(callbackHandle);

#if NODE_MODULE_VERSION >=14
	NanAddGCEpilogueCallback(afterGC);
#else
	V8::AddGCEpilogueCallback(afterGC);
#endif

	NanReturnUndefined();
}

void init(Handle<Object> exports) {
	NanScope();
#if NODE_MODULE_VERSION >=14
	NanAddGCPrologueCallback(recordBeforeGC);
#else
	V8::AddGCPrologueCallback(recordBeforeGC);
#endif

	exports->Set(NanSymbol("afterGC"), NanNew<FunctionTemplate>(AfterGC)->GetFunction());
}

NODE_MODULE(gcstats, init)