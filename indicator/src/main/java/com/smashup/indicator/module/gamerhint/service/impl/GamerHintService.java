package com.smashup.indicator.module.gamerhint.service.impl;

import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@RequiredArgsConstructor
public class GamerHintService {
    // 의존성 주입
//    private final MemberRepository memberRepository;

    // 전역 변수 참조
//    private final

    // 모듈 : 닉네임 수정
//    @Transactional
//    public void updateNickname(MemberEntity memberEntity) throws Exception {
//        // 필수 파라미터(DTO 대상) : (memberEntity에서 .get()하는 것들) : memberId, memberNickname
//        // entity 조회로 일관성 유지.
//        MemberEntity selectedMemberEntity = memberRepository.findById( memberEntity.getMemberId() )
//                .orElseThrow( ()-> new Exception() );
//        // 새 닉네임 set
//        selectedMemberEntity.setMemberNickname(memberEntity.getMemberNickname());
//        // member update
//        memberRepository.save(selectedMemberEntity);
//    }


}
