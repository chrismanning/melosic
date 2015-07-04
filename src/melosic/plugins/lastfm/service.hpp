/**************************************************************************
**  Copyright (C) 2012 Christian Manning
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef LASTFM_SERVICE_HPP
#define LASTFM_SERVICE_HPP

#include <string>
#include <memory>
#include <vector>

#include <pplx/pplxtasks.h>

#include <jbson/document.hpp>

#include <lastfm/lastfm.hpp>
#include <lastfm/detail/transform.hpp>
#include <lastfm/detail/params.hpp>

namespace lastfm {

struct user;
struct track;
struct tag;
struct artist;

class LASTFM_EXPORT service {
  public:
    using params_t = detail::params_t;

    explicit service(std::string_view api_key, std::string_view shared_secret);

    ~service();

    std::string_view api_key() const;
    std::string_view shared_secret() const;

    pplx::task<jbson::document> get(std::string_view method, params_t params);

    template <typename TransformerT, typename ReturnT = std::result_of_t<TransformerT(jbson::document)>>
    pplx::task<ReturnT> get(std::string_view method, params_t params, TransformerT&& transform);

  private:
    struct impl;
    std::unique_ptr<impl> pimpl;
};

template <typename TransformerT, typename ReturnT>
pplx::task<ReturnT> service::get(std::string_view method, params_t params, TransformerT&& transform) {
    return get(method, std::move(params)).then(transform);
}

namespace detail {

} // namespace detail

} // namespace lastfm

#endif // LASTFM_SERVICE_HPP
