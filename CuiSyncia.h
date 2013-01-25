#pragma once
//CuiSyncia:20130125
#include <iostream>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include "Neuria/Neuria.h"

namespace syncia
{
class CuiSyncia{
public:
    CuiSyncia(const neuria::network::HostName& host_name, 
			const neuria::network::PortNumber& port_num,
			const neuria::network::BufferSize& buffer_size) 
		: host_name(host_name), port_num(port_num), buffer_size(buffer_size),
			cui_shell(), io_service(neuria::network::IoService::Create()), 
			work(io_service->GetRawIoServiceRef()),
			command_dispatcher(
				neuria::command::AsyncExecuter([this](boost::function<void ()> func){
					this->io_service->GetRawIoServiceRef().post(func);
				}),
				neuria::command::OnFailedFunc()
			),
			client(io_service){}

	auto InitShell() -> void {
		neuria::test::RegisterExitFunc(this->cui_shell, "bye :)");		
		this->cui_shell.Register("link", "create link.", 
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				std::cout << boost::format("ConnectTo\"%1%:%2%\"") 
					% arg_list.at(1) % arg_list.at(2) 
				<< std::endl;
				this->client.Connect(
					neuria::network::HostName(arg_list.at(1)),
					neuria::network::PortNumber(boost::lexical_cast<int>(arg_list.at(2))),
					neuria::network::OnConnectedFunc(
							[this](neuria::network::Socket::Ptr socket){
						std::cout << "Connected!" << std::endl;	
						auto connection = neuria::network::Connection::Create(
							socket, this->buffer_size, 
							neuria::network::OnReceivedFunc([this](
									const neuria::network::ByteArray& byte_array){
								this->command_dispatcher.Dispatch(byte_array);
							}),
							neuria::network::Connection::OnPeerClosedFunc(
									[this](neuria::network::Connection::Ptr connection){
								//todo:erase connection from pool
							}),
							neuria::network::OnFailedFunc()
						);
						this->connection_pool.push_back(connection);
					}), 
					neuria::network::OnFailedFunc()
				);	
			})
		);
		this->cui_shell.Register("send", "send text",
			neuria::test::ShellFunc(
					[this](const neuria::test::CuiShell::ArgList& arg_list){
				auto wrapper = neuria::command::DispatchCommandWrapper(
					neuria::command::CommandId("message"), 
					neuria::network::CreateByteArrayFromString(arg_list.at(1))
				);
				for(auto connection : connection_pool){
					connection->Send(
						wrapper.Serialize(),
						neuria::network::OnSendedFunc(),
						neuria::network::OnFailedFunc()
					);
				}
			})
		);
	}

	auto InitDispatcher() -> void {
		this->command_dispatcher.RegisterFunc(
			neuria::command::CommandId("message"),
			neuria::command::OnReceivedFunc(
					[](const neuria::command::ByteArray& byte_array){
				std::cout << boost::format("received message:\"%1%\"")
					% neuria::network::CreateStringFromByteArray(byte_array) 
				<< std::endl;
			})
		);
	}

	auto Run() -> void {
		boost::thread_group thread_group;
		for(unsigned int i = 0; i < 10; ++i){
			thread_group.create_thread(
				boost::bind(&boost::asio::io_service::run, 
					&io_service->GetRawIoServiceRef())
			);
		}
		std::cout << boost::format("ServerReady:\"%1%:%2%\"") 
			% host_name.ToString() % port_num.ToInt()
		<< std::endl;
		neuria::network::Server server(this->io_service, this->port_num,
			neuria::network::OnAcceptedFunc(
					[this](neuria::network::Socket::Ptr socket){
				std::cout << "accepted!" << std::endl;
				auto connection = neuria::network::Connection::Create(
					socket, buffer_size, 
					neuria::network::OnReceivedFunc([this](
							const neuria::network::ByteArray& byte_array){
						this->command_dispatcher.Dispatch(byte_array);
					}),
					neuria::network::Connection::OnPeerClosedFunc(
							[this](neuria::network::Connection::Ptr connection){
						//todo:erase connection from pool
					}),
					neuria::network::OnFailedFunc()
				);
				this->connection_pool.push_back(connection);
				connection->StartReceive();
			}),
			neuria::network::OnFailedFunc([](
					const neuria::network::ErrorCode& error_code){
				
			})
		);
		server.StartAccept();
		cui_shell.Start();
		thread_group.join_all();
	}

private:
	neuria::network::HostName host_name;
	neuria::network::PortNumber port_num;
	neuria::network::BufferSize buffer_size;
	neuria::test::CuiShell cui_shell;
	neuria::network::IoService::Ptr io_service;
	boost::asio::io_service::work work;
	neuria::command::CommandDispatcher command_dispatcher;
	neuria::network::Client client;
	std::vector<neuria::network::Connection::Ptr> connection_pool;
};
}
