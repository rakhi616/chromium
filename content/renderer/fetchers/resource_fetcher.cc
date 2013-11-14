// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/fetchers/resource_fetcher.h"

#include "base/logging.h"
#include "third_party/WebKit/public/platform/Platform.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLLoader.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"

using base::TimeDelta;
using blink::WebFrame;
using blink::WebURLError;
using blink::WebURLLoader;
using blink::WebURLRequest;
using blink::WebURLResponse;

namespace content {

ResourceFetcher::ResourceFetcher(const GURL& url, WebFrame* frame,
                                 WebURLRequest::TargetType target_type,
                                 const Callback& callback)
    : completed_(false),
      callback_(callback) {
  // Can't do anything without a frame.  However, delegate can be NULL (so we
  // can do a http request and ignore the results).
  DCHECK(frame);
  Start(url, frame, target_type);
}

ResourceFetcher::~ResourceFetcher() {
  if (!completed_ && loader_)
    loader_->cancel();
}

void ResourceFetcher::SetTimeout(base::TimeDelta timeout) {
  DCHECK(loader_);
  DCHECK(!completed_);
  timeout_timer_.Start(FROM_HERE, timeout, this,
                       &ResourceFetcher::TimeoutFired);
}

void ResourceFetcher::Start(const GURL& url, WebFrame* frame,
                            WebURLRequest::TargetType target_type) {
  WebURLRequest request(url);
  request.setTargetType(target_type);
  request.setFirstPartyForCookies(frame->document().firstPartyForCookies());
  frame->dispatchWillSendRequest(request);

  loader_.reset(blink::Platform::current()->createURLLoader());
  loader_->loadAsynchronously(request, this);
}

void ResourceFetcher::RunCallback(const WebURLResponse& response,
                                  const std::string& data) {
  completed_ = true;
  timeout_timer_.Stop();
  if (callback_.is_null())
    return;

  // Take a reference to the callback as running the callback may lead to our
  // destruction.
  Callback callback = callback_;
  callback.Run(response, data);
}

void ResourceFetcher::TimeoutFired() {
  DCHECK(!completed_);
  loader_->cancel();
  RunCallback(WebURLResponse(), std::string());
}

/////////////////////////////////////////////////////////////////////////////
// WebURLLoaderClient methods

void ResourceFetcher::willSendRequest(
    WebURLLoader* loader, WebURLRequest& new_request,
    const WebURLResponse& redirect_response) {
}

void ResourceFetcher::didSendData(
    WebURLLoader* loader, unsigned long long bytes_sent,
    unsigned long long total_bytes_to_be_sent) {
}

void ResourceFetcher::didReceiveResponse(
    WebURLLoader* loader, const WebURLResponse& response) {
  DCHECK(!completed_);
  response_ = response;
}

void ResourceFetcher::didReceiveData(
    WebURLLoader* loader, const char* data, int data_length,
    int encoded_data_length) {
  DCHECK(!completed_);
  DCHECK(data_length > 0);

  data_.append(data, data_length);
}

void ResourceFetcher::didReceiveCachedMetadata(
    WebURLLoader* loader, const char* data, int data_length) {
  DCHECK(!completed_);
  DCHECK(data_length > 0);

  metadata_.assign(data, data_length);
}

void ResourceFetcher::didFinishLoading(
    WebURLLoader* loader, double finishTime) {
  DCHECK(!completed_);

  RunCallback(response_, data_);
}

void ResourceFetcher::didFail(WebURLLoader* loader, const WebURLError& error) {
  DCHECK(!completed_);

  // Go ahead and tell our delegate that we're done.
  RunCallback(WebURLResponse(), std::string());
}

}  // namespace content
