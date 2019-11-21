#include "uri.h"

using namespace rokid;
using namespace std;

#define URI_COUNT 7

static const char* test_uris[] = {
	"https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top",
	"mailto:John.Doe@example.com",
	"news:comp.infosystems.www.servers.unix",
	"tel:+1-816-555-1212",
	"telnet://192.0.2.16:80/",
	"urn:oasis:names:specification:docbook:dtd:xml:4.1.2",
	"unix:rokid-flora#foo"
};

typedef struct {
	string scheme;
	string user;
	string host;
	int32_t port = 0;
	string path;
	string query;
	string fragment;
} UriResult;

static UriResult correct_results[URI_COUNT];

static void prepare_correct_results() {
	correct_results[0].scheme = "https";
	correct_results[0].user = "john.doe";
	correct_results[0].host = "www.example.com";
	correct_results[0].port = 123;
	correct_results[0].path = "/forum/questions/";
	correct_results[0].query = "tag=networking&order=newest";
	correct_results[0].fragment = "top";
	correct_results[1].scheme = "mailto";
	correct_results[1].path = "John.Doe@example.com";
	correct_results[2].scheme = "news";
	correct_results[2].path = "comp.infosystems.www.servers.unix";
	correct_results[3].scheme = "tel";
	correct_results[3].path = "+1-816-555-1212";
	correct_results[4].scheme = "telnet";
	correct_results[4].host = "192.0.2.16";
	correct_results[4].port = 80;
	correct_results[4].path = "/";
	correct_results[5].scheme = "urn";
	correct_results[5].path = "oasis:names:specification:docbook:dtd:xml:4.1.2";
	correct_results[6].scheme = "unix";
	correct_results[6].path = "rokid-flora";
	correct_results[6].fragment = "foo";
}

static bool check_result(Uri& uri, UriResult& r) {
	if (uri.scheme != r.scheme) {
		printf("scheme incorrect: %s != %s\n", uri.scheme.c_str(), r.scheme.c_str());
		return false;
	}
	if (uri.user != r.user) {
		printf("user incorrect: %s != %s\n", uri.user.c_str(), r.user.c_str());
		return false;
	}
	if (uri.host != r.host) {
		printf("host incorrect: %s != %s\n", uri.host.c_str(), r.host.c_str());
		return false;
	}
	if (uri.port != r.port) {
		printf("port incorrect: %d != %d\n", uri.port, r.port);
		return false;
	}
	if (uri.path != r.path) {
		printf("path incorrect: %s != %s\n", uri.path.c_str(), r.path.c_str());
		return false;
	}
	if (uri.query != r.query) {
		printf("query incorrect: %s != %s\n", uri.query.c_str(), r.query.c_str());
		return false;
	}
	if (uri.fragment != r.fragment) {
		printf("fragment incorrect: %s != %s\n", uri.fragment.c_str(), r.fragment.c_str());
		return false;
	}
	return true;
}

int main(int argc, char** argv) {
	prepare_correct_results();

	int32_t i;
	Uri uri;
	for (i = 0; i < URI_COUNT; ++i) {
		if (!uri.parse(test_uris[i])) {
			printf("parse uri %s failed\n", test_uris[i]);
			return 1;
		}
		if (!check_result(uri, correct_results[i]))
			return 1;
	}
	printf("uri test success\n");
	return 0;
}
