require 'rubygems'
require 'sinatra'

get '/' do
  send_file "dash.html"
end
