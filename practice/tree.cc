#include <iostream>
#include <cstdint>
#include <vector>
using namespace std;
int main()
{
    int64_t n=16;
    vector<int64_t> sieve(n);
    for(int8_t i=0;i<n;++i)
        sieve[i]=i;
    int64_t size_ = n/2;
    vector<int64_t> tree_(size_);
    for (int64_t i = 0; i < size_; i++)
    {
        tree_[i] = sieve[i * 2];
        int64_t k = (i + 1) & ~i;
        for (int64_t j = i; k >>= 1; j &= j - 1){
            tree_[i] += tree_[j - 1];
            cout<<"j-1="<<j-1<<", k="<<k<<endl;
        }
    }
    for (int64_t i = 0; i < size_; i++)
    {
        cout<<"sieve["<<i<<"]="<<sieve[i]<<", tree_["<<i<<"]="<<tree_[i]<<endl;
    }

}