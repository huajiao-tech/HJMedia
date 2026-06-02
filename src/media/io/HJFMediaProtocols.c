//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJAsyncIO.h"
#include "HJFDataSourceIO.h"

//***********************************************************************************//
static const AVOption exasync_options[] = {
	{NULL},
};

static const AVClass exasync_context_class = {
	.class_name = "Exasync",
	.item_name = av_default_item_name,
	.option = exasync_options,
	.version = LIBAVUTIL_VERSION_INT,
};

const URLProtocol ff_exasync_protocol = {
	.name = "exasync",
	.url_open2 = exasync_open,
	.url_read = exasync_read,
	.url_seek = exasync_seek,
	.url_close = exasync_close,
	.priv_data_size = sizeof(HJExasyncContext),
	.priv_data_class = &exasync_context_class,
};

//***********************************************************************************//
static const AVOption hjds_options[] = {
	{NULL},
};

static const AVClass hjds_context_class = {
	.class_name = "HJDS",
	.item_name = av_default_item_name,
	.option = hjds_options,
	.version = LIBAVUTIL_VERSION_INT,
};

const URLProtocol ff_hjds_protocol = {
	.name = "hjds",
	.url_open2 = hjds_open,
	.url_read = hjds_read,
	.url_seek = hjds_seek,
	.url_close = hjds_close,
	.priv_data_size = sizeof(HJFDataSourceIOContext),
	.priv_data_class = &hjds_context_class,
};
