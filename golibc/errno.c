int errno = 0;

int* __get_errno_ptr();

int* __get_errno_ptr()
{
	return &errno;
}
