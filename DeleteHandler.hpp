/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DeleteHandler.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lottavi <lottavi@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/14 10:09:33 by lottavi           #+#    #+#             */
/*   Updated: 2025/11/14 10:09:34 by lottavi          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef DELETEHANDLER_HPP
#define DELETEHANDLER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include <string>

class DeleteHandler
{
public:
    static Response handle(const HttpRequest& req, const std::string& root_dir);
private:
    static bool deleteFile(const std::string& filepath);
};

#endif
