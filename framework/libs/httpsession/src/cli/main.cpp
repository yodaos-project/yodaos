#include "HttpSession.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <memory>

class RequestListener : public HttpSessionRequestListenerInterface
{
public:
    ~RequestListener()
    {
        printf("%s %p\n", __func__, this);
    }

    virtual void onRequestCanceled(HttpSession *session, HttpSession::Ticket *tic)
    {
    }

    virtual void onRequestFinished(HttpSession *session, HttpSession::Ticket *tic, HttpSession::Response *resp)
    {
        printf("\n==========\n%s %s %d %s\n==========\n", tic->request.url.c_str(), tic->request.method.c_str(),
               resp->code, resp->reason.c_str());
        if (tic->status() == HttpSession::Status::FAILED)
        {
            printf("Error: %d %s\n\n", tic->errorCode(), tic->errorMessage());
        }
        else
        {
            printf("%s\n\n", (const char *)resp->body);
        }
    }
};

using namespace std;

int parseHeaders(string &s, map<string, string> &headerMap)
{
    const char *k, *v, *temp = s.c_str(), *end = temp + s.size();
    int c = 0;
    while (temp < end)
    {
        k = temp;
        temp = index(k, ':');
        if (temp == NULL)
        {
            headerMap[string(k, end - k)] = "";
            ++c;
            break;
        }
        v = temp + 1;
        temp = index(v, ',');
        if (temp == NULL)
        {
            headerMap[string(k, v - k - 1)] = string(v, end - v);
            ++c;
            break;
        }
        headerMap[string(k, v - k - 1)] = string(v, temp - v);
        ++c;
        ++temp;
    }
    printf("%s %d\n", __func__, c);
    return c;
}

static void ticketDeleter(HttpSession::Ticket *tic)
{
    printf("%s %p\n", __func__, tic);
}

int main(int argc, char **argv)
{
    string method, headers, body, bodyfile;
    int opt, serial = 0, count = 1, timeout = 10, verbose = 0, verify = 1;

    while ((opt = getopt(argc, argv, "m:h:b:B:c:t:Vsn")) != -1)
    {
        switch (opt)
        {
            case 'm':
                method.assign(optarg);
                break;
            case 'h':
                headers.assign(optarg);
                break;
            case 'b':
                body.assign(optarg);
                break;
            case 'B':
                bodyfile.assign(optarg);
                break;
            case 'c':
                count = atoi(optarg);
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case 's':
                serial = 1;
                break;
            case 'n':
                verify = 0;
                break;
            case 'V':
                verbose = 1;
                break;
            default:
                break;
        }
    }

    auto session = new HttpSession({ "", 30, verbose });
    auto req = HttpSession::Request();
    if (!method.empty())
    {
        req.method = method;
    }
    parseHeaders(headers, req.headers);
    if (!body.empty())
    {
        req.body = body.c_str();
    }
    if (!verify)
    {
        req.verifyPolicy = HttpSession::VERIFY_NONE;
    }

    RequestListener listener;
    if (serial)
    {
        for (int i = optind; i < argc; ++i)
        {
            req.url.assign(argv[i]);
            for (int j = 0; j < count; ++j)
            {
                printf("\n>>>>> request %s(%d) <<<<<\n", argv[i], j + 1);
                req.headers["Time"] = to_string(j + 1);
                auto tic = session->request(req, &listener, ticketDeleter);

                sleep(timeout);
                printf("release ticket %p %p\n", tic, session);
            }
        }
    }
    else
    {
        for (int i = 0; i < count; ++i)
        {
            vector<shared_ptr<HttpSession::Ticket>> tics;
            for (int j = optind; j < argc; ++j)
            {
                req.url.assign(argv[j]);
                req.headers["Time"] = to_string(i + 1);
                tics.push_back(session->request(req, &listener, ticketDeleter));
            }
            sleep(timeout);
        }
    }

    printf("release session\n");
    delete session;
    return 0;
}
