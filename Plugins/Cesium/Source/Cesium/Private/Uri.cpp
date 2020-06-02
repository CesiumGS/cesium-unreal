#include "Uri.h"
#define URI_STATIC_BUILD
#include "uriparser/Uri.h"

std::string Uri::resolve(const std::string& base, const std::string& relative, bool useBaseQuery)
{
	UriUriA baseUri;

	if (uriParseSingleUriA(&baseUri, base.c_str(), nullptr) != URI_SUCCESS)
	{
		// Could not parse the base, so just use the relative directly and hope for the best.
		return relative;
	}

	UriUriA relativeUri;
	if (uriParseSingleUriA(&relativeUri, relative.c_str(), nullptr) != URI_SUCCESS)
	{
		// Could not parse one of the URLs, so just use the relative directly and hope for the best.
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	UriUriA resolvedUri;
	if (uriAddBaseUriA(&resolvedUri, &relativeUri, &baseUri) != URI_SUCCESS) {
		uriFreeUriMembersA(&resolvedUri);
		uriFreeUriMembersA(&relativeUri);
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	if (uriNormalizeSyntaxA(&resolvedUri) != URI_SUCCESS)
	{
		uriFreeUriMembersA(&resolvedUri);
		uriFreeUriMembersA(&relativeUri);
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	int charsRequired;
	if (uriToStringCharsRequiredA(&resolvedUri, &charsRequired) != URI_SUCCESS) {
		uriFreeUriMembersA(&resolvedUri);
		uriFreeUriMembersA(&relativeUri);
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	std::string result(charsRequired, ' ');

	if (uriToStringA(const_cast<char*>(result.c_str()), &resolvedUri, charsRequired + 1, nullptr) != URI_SUCCESS) {
		uriFreeUriMembersA(&resolvedUri);
		uriFreeUriMembersA(&relativeUri);
		uriFreeUriMembersA(&baseUri);
		return relative;
	}

	if (useBaseQuery)
	{
		std::string query(baseUri.query.first, baseUri.query.afterLast);
		std::cout << query << std::endl;
		if (resolvedUri.query.first)
		{
			result += "&" + query;
		}
		else
		{
			result += "?" + query;
		}
	}

	uriFreeUriMembersA(&resolvedUri);
	uriFreeUriMembersA(&relativeUri);
	uriFreeUriMembersA(&baseUri);

	return result;
}

std::string Uri::addQuery(const std::string& uri, const std::string& key, const std::string& value)
{
	// TODO
	return uri + "&" + key + "=" + value;
	//UriUriA baseUri;

	//if (uriParseSingleUriA(&baseUri, uri.c_str(), nullptr) != URI_SUCCESS)
	//{
	//	// TODO: report error
	//	return uri;
	//}

	//uriFreeUriMembersA(&baseUri);
}