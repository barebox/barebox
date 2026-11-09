// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/macros/concat_idents.rs?id=05f7e89ab9731565d8a62e3b5d1ec206485eeb0b

use proc_macro::{token_stream, Ident, TokenStream, TokenTree};

use crate::helpers::expect_punct;

fn expect_ident(it: &mut token_stream::IntoIter) -> Ident {
    if let Some(TokenTree::Ident(ident)) = it.next() {
        ident
    } else {
        panic!("Expected Ident")
    }
}

pub(crate) fn concat_idents(ts: TokenStream) -> TokenStream {
    let mut it = ts.into_iter();
    let a = expect_ident(&mut it);
    assert_eq!(expect_punct(&mut it), ',');
    let b = expect_ident(&mut it);
    assert!(it.next().is_none(), "only two idents can be concatenated");
    let res = Ident::new(&format!("{a}{b}"), b.span());
    TokenStream::from_iter([TokenTree::Ident(res)])
}
