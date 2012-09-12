//
// AccountManager.cpp
//

#include "AccountManager.hpp"
#include "../common/database/AccountProperty.hpp"
#include "../common/network/Utils.hpp"
#include "../common/network/Encrypter.hpp"
#include "../common/unicode.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>

AccountManager::AccountManager(const ManagerAccessorPtr& manager_accessor) :
manager_accessor_(manager_accessor)
{
}

void AccountManager::Load(const std::string& filename)
{
    using boost::property_tree::ptree;
    ptree pt;

    namespace fs = boost::filesystem;
    if (fs::exists(filename)) {
        read_xml(filename, pt);
    }

    name_ = pt.get<std::string>("name", "???");
    trip_ = pt.get<std::string>("trip", "");
    model_name_ = pt.get<std::string>("model_name", unicode::ToString(_T("char:初音ミク")));
    udp_port_ = pt.get<uint16_t>("udp_port", 39391);
    host_ = pt.get<std::string>("host", "127.0.0.1");

    public_key_ = network::Utils::Base64Decode(pt.get<std::string>("public_key", ""));
    private_key_ = network::Utils::Base64Decode(pt.get<std::string>("private_key", ""));

    if (public_key_.empty() || private_key_.empty()) {
        network::Encrypter encrypter;
        public_key_ = encrypter.GetPublicKey();
        private_key_ = encrypter.GetPrivateKey();
    }

    Save(filename);
}

void AccountManager::Save(const std::string& filename) const
{
    using boost::property_tree::ptree;
    ptree pt;

    pt.add("name", name_);
    pt.add("trip", trip_);
    pt.add("model_name", model_name_);
    pt.add("udp_port", udp_port_);
    pt.add("host", host_);

    pt.add("public_key", network::Utils::Base64Encode(public_key_));
    pt.add("private_key", network::Utils::Base64Encode(private_key_));

    write_xml(filename, pt);
}

std::string AccountManager::GetSerializedData() const
{
    std::string data;
    if (name_.size() > 0 && name_.size() <= 16) {
        data += network::Utils::Serialize((uint16_t)AccountProperty::NAME, name_);
    }
    if (trip_.size() > 0 && trip_.size() <= 16) {
        data += network::Utils::Serialize((uint16_t)AccountProperty::TRIP, trip_);
    }
    if (model_name_.size() > 0 && model_name_.size() <= 64) {
        data += network::Utils::Serialize((uint16_t)AccountProperty::MODEL_NAME, model_name_);
    }
    data += network::Utils::Serialize((uint16_t)AccountProperty::UDP_PORT, udp_port_);

    return data;
}

std::string AccountManager::public_key() const
{
    return public_key_;
}

std::string AccountManager::private_key() const
{
    return private_key_;
}

std::string AccountManager::name() const
{
    return name_;
}

void AccountManager::set_name(const std::string& name)
{
    name_ = name;
}

std::string AccountManager::model_name() const
{
    return model_name_;
}

void AccountManager::set_model_name(const std::string& name)
{
    model_name_ = name;
}

uint16_t AccountManager::udp_port() const
{
    return udp_port_;
}

void AccountManager::set_udp_port(uint16_t port)
{
    udp_port_ = port;
}

std::string AccountManager::host() const
{
    return host_;
}

void AccountManager::set_host(const std::string& host)
{
    host_ = host;
}
