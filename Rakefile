require 'rake/clean'
require 'rake/packagetask'

# バージョンナンバー
PLUGIN_VERSION = IO.readlines("Info.plist")[0].match(/(\d|\.)+/)

# rake clean で build ディレクトリ以下を削除
CLEAN.include("build")

# rake package で zip ファイルを作る
package = Rake::PackageTask.new("dat", PLUGIN_VERSION) do |p|
	p.package_dir = "./zip"
	p.package_files.include("dat.qlgenerator")
	p.package_files.exclude(".DS_Store")
	p.need_zip = true
end

desc "Install plugin"

task :install do
	system('xcodebuild -configuration Release -target "Install plugin for user"')
end

desc "Build for BathyScaphe"

task :bathyscaphe do
	system('xcodebuild -configuration Release -target "Build for BathyScaphe"')
end

desc "Build standard Quick Look plugin"

task :default do
	system('xcodebuild -configuration Release -target "dat"')
end

# Copy dat.qlgenerator
file "dat.qlgenerator" do
	if !File.exist?("build/Release/dat.qlgenerator") # dat.qlgenerator がない場合はビルドする
		puts "File not exist. Build Start."
		Rake::Task[:default].invoke()
	end
	
	sh "mkdir -p #{package.package_dir_path}"
	sh "cp -R build/Release/dat.qlgenerator #{package.package_dir_path}"
	sh "cp *.txt #{package.package_dir_path}"
end