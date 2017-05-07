/* copyright(C) 2003 H.Kawai (under KL-01). */

unsigned int rand_seed = 8;

int rand(void)
{
	rand_seed ^= (rand_seed << 13);
	rand_seed ^= (rand_seed >> 9);
	return rand_seed ^= (rand_seed << 7);
}
