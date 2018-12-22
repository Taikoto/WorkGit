########################################################################################################
# ParryUtil 类, 提供实用函数, 如: 输出结果文件, 输出 Log 
########################################################################################################
# PARRY 13:57 2014-5-16 
# 增加通用清屏函数 ClearScreen; 
# output_message 在 Linux 系统下可以正常显示中文字符

package ParryUtil;
require Exporter;
@ISA = qw(Exporter);

# @EXPORT 可被外部程序访问的函数, 其它未定义的都是私有, 不可被外部访问
@EXPORT = qw(set_log_handle set_result_handle output_message log output_result 
            print_to_file open_text_file ltrim rtrim subtrim 
            isLinux isWin32 ClearScreen getSysUserName WordEncode);
# @EXPORT_OK = qw($myvar1 $myvar2);	 # 可被外部程序访问的变量

use Data::Dumper;
use Carp qw(confess);               # 产生调用错误的子程序命令行位置
use Scalar::Util qw/openhandle/;    # 检查文件柄是否已经打开

use utf8;
use Encode qw/encode decode is_utf8/;

sub new
{
    my $class = shift;
    my $self = bless {}, $class;
    $self->_init;
    return $self;
}

sub _init
{
    my $self = shift;
    $self->{logfile}        = '';       # 输出调试信息的文件
    $self->{handle_log_file} = undef;    # 输出调试信息的文件句柄
    $self->{handle_result_file} = undef;    # 输出调试信息的文件句柄
}

# 设置输出调试信息的文件句柄
sub set_log_handle
{
	my ( $self, $fh) = @_;

	$self->{handle_log_file} = $fh;
}

# 设置输出结果的文件句柄
sub set_result_handle
{
	my ( $self, $fh) = @_;

	$self->{handle_result_file} = $fh;
}

#####################
# 实用函数
#####################

# 在命令窗口输出消息
sub output_message
{
	my($self, $c_string) = @_;		# 获得传入参数
    my $l_string = $c_string;
    
    $l_string = encode("gbk",$c_string) if (isWin32());         # Windows 操作系统中避免启用 utf8 状态下乱码
    Encode::_utf8_off($l_string) if (isLinux());		          # Linux 操作系统中避免出现提示宽字符警告
    
	print $l_string; 
}

# 在文件输出调试消息, 使用此函数前必须先调用 set_log_handle() 设置 $self->{handle_log_file}
sub log
{
	my ($self, $p_string) = @_;
	my $fh = $self->{handle_log_file};

	if (Scalar::Util::openhandle($fh) == undef)
	{
		warn 'File handle undef! plase set log handle.';
		return;
	}

	# print_to_file($p_string, $fh);

	Encode::_utf8_off($p_string);		# 避免出现提示宽字符警告
	print $fh $p_string;
}

# 在文件输出调试消息, 使用此函数前必须先调用 set_result_handle() 设置 $self->{handle_result_file}
sub output_result
{
	my ($self, $p_string) = @_;
	my $fh = $self->{handle_result_file};

	if (Scalar::Util::openhandle($fh) == undef)
	{
		warn 'File handle undef! plase set log handle.';
		return;
	}

	# print_to_file($p_string, $fh);

	Encode::_utf8_off($p_string);		# 避免出现提示宽字符警告
	print $fh $p_string;
}

# 通用文件输出
sub print_to_file
{
	my ($self, $p_string, $fh) = @_;

	if (Scalar::Util::openhandle($fh) == undef)
	{
		warn 'File handle undef! plase open file.';
		return;
	}

	Encode::_utf8_off($p_string);		# 避免出现提示宽字符警告
	print $fh $p_string;
}

# 打开文件, 支持双字节中文文件名称
sub open_text_file
{
	my($self, $mode, $fname) = @_;		# 获得传入参数
	my $fh = undef;

	# $mode .= ':encoding(utf8)';

	# 文件名是通过标准输出传给命令行的，而命令行的编码是gbk，所以要编码成gbk，保持一致
	$fname = encode("gbk", $fname);

	open($fh, $mode, $fname) or confess "Cannot open file '$fname' $!";
	return $fh;
}

#####################
# 字符处理函数
#####################

# 清除字符串左边空字符
sub ltrim
{
	my ($self, $p_string) = @_;

	$p_string =~ s/^\s+//;
	return $p_string;
}

# 清除字符串左边空字符
sub rtrim
{
	my ($self, $p_string) = @_;

	$p_string =~ s/\s+$//;
	return $p_string;
}

# 清除字符串左右两边空字符
sub subtrim
{
	my ($self, $p_string) = @_;

	$p_string =~ s/^\s+//;
	$p_string =~ s/\s+$//;
	return $p_string;
}

# 当前操作系统是: Linux
sub isLinux
{
    return ($^O =~ /linux/i);
}

# 当前操作系统是: Windows
sub isWin32
{
    return ($^O =~ /MSWin32/i);
}

# 清空命令窗口
sub ClearScreen
{
    if (isLinux())
    {
        system("clear");        # for linux
    }
    
    if (isWin32())
    {
        system("cls");        # for windows
    }
}

# 获取操作系统用户名称
sub getSysUserName
{
    my $user_name = '';
    if (isLinux())
    {
        $user_name = $ENV{'USER'};        # for linux
    }
    
    if (isWin32())
    {
        $user_name = $ENV{'USERNAME'};        # for windows
    }
    
    return $user_name;
}

#####################
# 字符编码函数
#####################
sub WordEncode{
	my($char);
	$char = @_[0];
	$char=decode ("gbk",$char);  # 转换为 utf8编码 字符 	# gb2312 # gbk, utf8, big5
	unpack ("U*",$char);  # 转换为 utf8编码 字符
}

return 1;	# 必须的返回命令