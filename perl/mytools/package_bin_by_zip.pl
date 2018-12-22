#!/usr/bin/perl

# 打包 bin 为 zip 文件, 名称例如: j7296_20140507.zip
# 传入参数为“0”时，不打包数据库
# PARRY 19:51 2014-5-7
# PARRY 13:50 2014-5-16 增加打包数据库文件 
# PARRY 9:40 2014-7-11 改为调用外部库形式(ParryMTKAndroidUtil.pm)

# 增加附加的类库搜索路径, ParryMTKAndroidUtil, ParryUtil 所在目录
BEGIN { push @INC, './mytools' }  
 
use ParryMTKAndroidUtil;	# 提供 MTK Android 实用函数
use ParryUtil;					# 提供实用函数, 如: 输出结果文件, 输出 Log 

use File::Spec::Functions;
use Cwd;

use utf8;
use Encode qw/encode decode is_utf8/;

ParryUtil::ClearScreen();       # 清屏
################
my $g_util = ParryUtil->new();         	 # 提供实用函数
my $curdir = getcwd();      # 当前脚本目录

my $ini = 'makeMtk.ini';
my $prj_name = 'YQ02';         # 打包需要的名称
my $prj_name_mtk = 'nb6735m_65u_l1';     # 代码中的项目目录名称
my $fname_scatter = 'MT6735M_Android_scatter.txt';   # Scatter 文件名称
my $dir_bin_base = './out/target/product';          # Android 输出的基本目录
my $dir_out = './';             # 输出目录 
my $include_database = 1;       # 包含数据库文件
my $build_mode_mtk = 'userdebug';

# 处理操作系统传入的参数
my $g_para = 1;
if ($#ARGV >= 0)	# 最少有 1 个参数
{
	$g_para = shift(@ARGV);			# 提取第一个参数, 并将其从 @ARGV 删除
    $include_database = 0 if ($g_para == 0);
}
output_message("提示：传入参数为“0”时，不打包数据库");

if (-e $ini)
{
    open (FILE_HANDLE, "<$ini") or die "cannot open $ini\n";
    while (<FILE_HANDLE>)
    {
        if (/^(\S+)\s*=\s*(\S+)/)
        {
            $keyname = $1;
            $${keyname} = $2;
        }
    }
    close FILE_HANDLE;
    
    if (defined($project))
    {
        $prj_name_mtk = lc($project);
        $prj_name = $prj_name_mtk;
    }
    
    if (defined($build_mode))
    {
        $build_mode_mtk = $build_mode;
    }
    package_bin($prj_name, $prj_name_mtk, $dir_bin_base, $dir_out, $fname_scatter, $build_mode_mtk, $include_database);
}
else
{
	package_bin($prj_name, $prj_name_mtk, $dir_bin_base, $dir_out, $fname_scatter, $build_mode_mtk, $include_database);
#    output_message("文件 " . catfile($curdir, $ini) . " 不存在, 请编译后执行。");
}


exit 1;


sub output_message
{
    my ($msg) = @_;
    $g_util->output_message("$msg\n");
}
