require 'rubygems'
require 'sinatra'
require 'httparty'

get '/' do
  send_file "dash.html"
end

get '/:args' do |args|
  HTTParty.get("http://192.168.1.4/#{args}")
end

post '/pump/:state' do |state|
    HTTParty.post("http://192.168.1.4/pump/#{state}",{})
end