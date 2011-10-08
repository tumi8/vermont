#ifdef PLUGIN_SUPPORT_ENABLED
#include "BannerOSMapping.h"
#include <cassert>
#include "common/msg.h"
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/algorithm/string/find.hpp>

typedef const boost::iterator_range<std::string::const_iterator> StringRange;

BannerOSMapping::BannerOSMapping():osType(""), osVersion(""), bannertype(HTTP), banner(""){
}

BannerOSMapping::BannerOSMapping(const std::string osType, const std::string osVersion,
                                 const std::string banner, const e_bannerType bannertype){
    this->osType = osType;
    this->osVersion = osVersion;
    this->banner = banner;
    this->bannertype = bannertype;
}

bool BannerOSMapping::osTypeMatches(std::string other) const{
    if ( boost::ifind_first(
             StringRange(other.begin(), other.end()),
             StringRange(osType.begin(), osType.end()) ) )
    {
        return true;
    }
    return false;
}

bool BannerOSMapping::osVersionMatches(std::string other) const{
    if ( boost::ifind_first(
             StringRange(other.begin(), other.end()),
             StringRange(banner.begin(), banner.end()) ) )
    {
        return true;
    }
    return false;
}

std::string BannerOSMapping::findArchitecture(std::string other) const{
    if ( other.find("x86_64") != std::string::npos ){
        return "x86_64";
    }
    if ( other.find("i686") != std::string::npos ){
        return "i686";
    }
    if ( other.find("i386") != std::string::npos ){
        return "i386";
    }
    return "";
}

std::list<BannerOSMapping> BannerOSMapping::getBanners(std::string fileName){
    std::list<BannerOSMapping> banners;
    XMLDocument* document = XMLDocument::parse_file(fileName);
    XMLElement* root = document->getRootNode();

    // consistency checks
    if (!root) {
        THROWEXCEPTION("%s is an empty XML-Document!", fileName.c_str());
    }

    if (!root->matches("bannerOSMapping")) {
        THROWEXCEPTION("Root element does not match \"bannerOSMapping\"."
                       " This is not a valid Banner file!");
    }

    XMLNode::XMLSet<XMLElement*> rootElements = root->getElementChildren();
    for (XMLNode::XMLSet<XMLElement*>::const_iterator it = rootElements.begin();
         it != rootElements.end();
         it++) {
        XMLElement* e = *it;
        if (e->matches("mapping")) {
            banners.push_back(*(readMapping(e)));
        } else {
            msg(MSG_FATAL, "Unkown Banner file entry %s\n", e->getName().c_str());
        }
    }

    return banners;
}

BannerOSMapping* BannerOSMapping::readMapping(XMLElement* elem){
    BannerOSMapping* banner = new BannerOSMapping();
    XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
    for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
         it != set.end();
         it++) {
        XMLElement* e = *it;

        if (e->matches("osType")) {
            banner->osType = e->getFirstText();
        } else if (e->matches("osVersion")){
            banner->osVersion = e->getFirstText();
        } else if (e->matches("banner")){
            banner->banner = e->getFirstText();
        } else if (e->matches("bannerType")){
            std::string type = e->getFirstText();
            if (type == "HTTP") banner->bannertype = HTTP;
            if (type == "SSH") banner->bannertype = SSH;
        } else {
            msg(MSG_INFO, "Unkown XMLElement in Banner file %s\n", e->getName().c_str());
        }
    }
    return banner;
}


double BannerOSMapping::computeStringSimilarity(const std::string source, const std::string target) const{
    // compute Levenshtein Algorithm
    using namespace boost::numeric::ublas;
    const int m = source.length();
    const int n = target.length();
    if (m == 0) {
        return n;
    }
    if (n == 0) {
        return m;
    }

    matrix<int> mat (m+1, n+1);

    // initialize values 0,1,2,3,...
    for (int i = 0; i <= m; i++){
        mat(i, 0) = i; // the distance of any first string to an empty second string
    }
    for (int j = 0; j <= n; j++){
        mat(0, j) = j; // the distance of any second string to an empty first string
    }

    for (int j = 1; j <= n; j++){
        const char t_i = target[j-1];
        for (int i = 1; i <= m; i++){
            const char s_i = source[i-1];
            if (s_i == t_i){
                mat(i, j) = mat(i-1, j-1); // no operation required
            } else {
                mat(i, j) = std::min( mat(i-1, j) + 1, // a deletion
                                      std::min( mat(i, j-1) + 1, // an insertion
                                                mat(i-1, j-1) + 1 )); // a substitution
            }
        }
    }
    return 1 - (mat(m, n) / double(std::max(m, n)));
}
#endif
